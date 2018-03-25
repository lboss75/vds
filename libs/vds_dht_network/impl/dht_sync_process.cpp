/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <messages/offer_replica.h>
#include <messages/replica_not_found.h>
#include "stdafx.h"
#include "private/dht_sync_process.h"
#include "channel_local_cache_dbo.h"
#include "dht_network_client.h"
#include "messages/channel_log_state.h"
#include "transaction_log_record_dbo.h"
#include "chunk_replica_map_dbo.h"
#include "private/dht_network_client_p.h"
#include "messages/offer_move_replica.h"
#include "chunk_replica_data_dbo.h"

void vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {

  this->sync_local_channels(sp, t);
  this->sync_replicas(sp, t);

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
            client->current_node_id()));

    std::shared_lock<std::shared_mutex> lock(this->channel_subscribers_mutex_);
    auto p_channel = this->channel_subscribers_.find(p.first);
    if (this->channel_subscribers_.end() != p_channel) {
      for (const auto & node : p_channel->second) {
        client->send(
            sp,
            node,
            messages::channel_log_state(
                p.first,
                p.second,
                client->current_node_id()).serialize());
      }
    }

  }
}

struct object_info{
  std::map<uint16_t /*replica*/,
      std::map<vds::const_data_buffer /*distance*/,
          std::list<vds::const_data_buffer/*node_id*/>>> replicas;
  std::map<vds::const_data_buffer/*node_id*/, std::list<uint16_t>/*replica*/> nodes;
};

void vds::dht::network::sync_process::sync_replicas(
    const vds::service_provider &sp,
    vds::database_transaction &t) {
  auto & client = *sp.get<dht::network::client>();

  std::map<const_data_buffer /*object_id*/, object_info> objects;
  orm::chunk_replica_map_dbo t1;
  auto st = t.get_reader(t1.select(t1.id, t1.replica, t1.node));
  while(st.execute()){
    auto object_id = base64::to_bytes(t1.id.get(st));
    auto node_id = base64::to_bytes(t1.node.get(st));
    objects[object_id].replicas[t1.replica.get(st)]
    [dht_object_id::distance(object_id, node_id)].push_back(node_id);
    objects[base64::to_bytes(t1.id.get(st))].nodes[node_id].push_back(t1.replica.get(st));
  }

  //Move replicas closer to root
  for(auto & p : objects){
    std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> neighbors;
    client->neighbors(sp, p.first, neighbors, _client::GENERATE_HORCRUX);

    for(auto & pneighbor : neighbors){
      for(auto & pneighbor_node : pneighbor.second) {
        if (p.second.nodes.end() == p.second.nodes.find(pneighbor_node))
          for (auto &preplica : p.second.replicas) {
            for (auto &pnode : preplica.second) {
              if (pneighbor.first < pnode.first) {
                for(auto & node : pnode.second) {
                 client.send(sp, pneighbor_node, messages::offer_move_replica(
                      p.first,
                      preplica.first,
                      node,
                      client.current_node_id()));
                }
              }
            }
          }
      }
    }
  }

  //
}

void vds::dht::network::sync_process::apply_message(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::dht::messages::offer_move_replica &message) {

  auto & client = *sp.get<dht::network::client>();

  orm::chunk_replica_data_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.replica_data)
          .where(t1.id == base64::from_bytes(message.object_id())
          && t1.replica == message.replica()));
  if(!st.execute()){
    client.send(sp, message.source_node(), messages::replica_not_found(
        message.object_id(),
        message.replica(),
        client.current_node_id()));
  }
  else {
    client.send(sp, message.target_node(), messages::offer_replica(
        message.object_id(),
        message.replica(),
        t1.replica_data.get(st),
        message.source_node(),
        client.current_node_id()));
  }
}

void vds::dht::network::sync_process::apply_message(const vds::service_provider &sp, vds::database_transaction &t,
                                                    const vds::dht::messages::replica_not_found &message) {
  orm::chunk_replica_map_dbo t1;
  t.execute(t1.delete_if(
      t1.id == base64::from_bytes(message.object_id())
      && t1.replica == message.replica()
      && t1.node == base64::from_bytes(message.source_node())));

}

void vds::dht::network::sync_process::apply_message(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::dht::messages::offer_replica &message) {

  orm::chunk_replica_map_dbo t1;
  t.execute(t1.insert_or_ignore(
      t1.id = base64::from_bytes(message.object_id()),
      t1.replica = message.replica(),
      t1.node = base64::from_bytes(message.source_node())));

  orm::chunk_replica_data_dbo t2;
  auto st = t.get_reader(t2.select(t2.replica).where(t2.id == base64::from_bytes(message.object_id())));
  if(!st.execute()){
    t.execute(t2.insert(
        t2.id = base64::from_bytes(message.object_id()),
        t2.replica = message.replica(),
        t2.replica_data = message.replica_data()
    ));

    auto & client = *sp.get<client>();
    client.send(sp, message.source_node(), messages::got_replica(
       message.object_id(),
       message.replica(),
       client.current_node_id()));

    std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> neighbors;
    client->neighbors(sp, message.object_id(), neighbors, _client::GENERATE_HORCRUX);

    for(auto & p : neighbors){
      for(auto & node : p.second) {
        client.send(sp, node, messages::got_replica(
            message.object_id(),
            message.replica(),
            client.current_node_id()));
      }
    }
  }

}

void vds::dht::network::sync_process::apply_message(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::dht::messages::got_replica &message) {

  orm::chunk_replica_map_dbo t1;
  t.execute(t1.insert_or_ignore(
      t1.id = base64::from_bytes(message.object_id()),
      t1.replica = message.replica(),
      t1.node = base64::from_bytes(message.source_node())));

}
