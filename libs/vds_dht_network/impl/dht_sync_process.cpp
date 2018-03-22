/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_sync_process.h"
#include "channel_local_cache_dbo.h"
#include "dht_network_client.h"
#include "messages/channel_log_state.h"
#include "transaction_log_record_dbo.h"

void vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {

  this->sync_local_channels(sp, t);

}

void vds::dht::network::sync_process::sync_local_channels(const service_provider& sp, database_transaction& t) {
  auto client = sp.get<dht::network::client>();
  
  orm::channel_local_cache_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.channel_id)
        .where(t1.last_sync > std::chrono::system_clock::now() - std::chrono::hours(24 * 30)));

  std::map<const_data_buffer, std::list<const_data_buffer>> channels;
  while (st.execute()) {
    channels.emplace(base64::to_bytes(t1.channel_id.get(st)));
  }

  orm::transaction_log_record_dbo t2;
  st = t.get_reader(
      t2.select(t2.channel_id, t2.id)
          .where(t2.state == (int)orm::transaction_log_record_dbo::state_t::leaf));

  while (st.execute()) {
    channels[base64::to_bytes(t2.channel_id.get(st))].push_back(base64::to_bytes(t2.id.get(st)));
  }

  for(auto p : channels){

    std::string log_message;
    for (const auto & r : p.second) {
      log_message += base64::from_bytes(r);
      log_message += ' ';
    }

    sp.get<logger>()->trace(
        ThisModule,
        sp,
        "Channel %s state is %s",
        base64::from_bytes(p.first).c_str(),
        log_message.c_str());

    client->send(
        sp,
        p.first,
        messages::channel_log_state(
            p.first,
            p.second,
            client->current_node_id()).serialize());

    std::shared_lock<std::shared_mutex> lock(this->channel_subscribers_mutex_);
    auto p_channel = this->channel_subscribers_.find(p.first);
    if (this->channel_subscribers_.end() != p_channel) {
      for (const auto & node : p_channel->second) {
        p2p->send(
            sp,
            node,
            messages::channel_log_state(
                p.first,
                p.second,
                p2p->current_node_id()).serialize());
      }
    }

  }
}
