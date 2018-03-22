/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_sync_process.h"
#include "channel_local_cache_dbo.h"
#include "dht_network_client.h"
#include "messages/channel_log_state.h"

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

  std::set<const_data_buffer> channels;
  while (st.execute()) {
    channels.emplace(base64::to_bytes(t1.channel_id.get(st)));
  }

  for(auto channel_id : channels){
    client->send(
      sp,
      channel_id,
      vds::dht::messages::channel_log_state(channel_id, ));
  }
}
