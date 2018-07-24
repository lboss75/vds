/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/sync_process.h"
#include "dht_network_client.h"
#include "transaction_log_record_dbo.h"
#include "chunk_dbo.h"
#include "transaction_log_unknown_record_dbo.h"
#include "transaction_log.h"
#include "chunk_replica_data_dbo.h"
#include "sync_replica_map_dbo.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "sync_message_dbo.h"
#include "db_model.h"
#include "device_config_dbo.h"

void vds::transaction_log::sync_process::do_sync(const service_provider& sp, database_transaction& t) {
  this->sync_local_channels(sp, t);
  this->sync_replicas(sp, t);
  this->query_unknown_records(sp, t);
}

void vds::transaction_log::sync_process::query_unknown_records(const service_provider& sp, database_transaction& t) {
  std::map<const_data_buffer, std::vector<const_data_buffer>> record_ids;
  orm::transaction_log_unknown_record_dbo t1;
  orm::transaction_log_unknown_record_dbo t2;
  auto st = t.get_reader(
    t1.select(
      t1.id,
      t1.refer_id)
    .where(db_not_in(t1.id, t2.select(t2.follower_id))));

  while (st.execute()) {
    record_ids[base64::to_bytes(t1.id.get(st))].push_back(t1.refer_id.get(st));
  }

  if(record_ids.empty()){
    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(
        t2.select(t2.id)
            .where(t2.state == (int)orm::transaction_log_record_dbo::state_t::leaf));

    std::list<const_data_buffer> current_state;
    while (st.execute()) {
      auto id = base64::to_bytes(t2.id.get(st));
      current_state.push_back(id);
    }

    if(!current_state.empty()) {
        auto & client = *sp.get<vds::dht::network::client>();
        client->send_neighbors(
            sp,
            messages::transaction_log_state(
                current_state,
                client->current_node_id()));
    }
  }
  else {
    auto &client = *sp.get<dht::network::client>();
    for (const auto &p : record_ids) {
      auto index = std::rand() % p.second.size();
      sp.get<logger>()->trace(
          ThisModule,
          sp,
          "Query log records %s from %s",
          base64::from_bytes(p.first).c_str(),
          base64::from_bytes(p.second[index]).c_str());

        client->send(
            sp,
          p.second[index],
            messages::transaction_log_request(
              p.first,
                client->current_node_id()));
    }
  }
}


vds::async_task<> vds::transaction_log::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const dht::messages::transaction_log_state& message) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  for (auto & p : message.leafs()) {
    auto st = t.get_reader(
      t1.select(t1.state)
      .where(t1.id == base64::from_bytes(p)));
    if (!st.execute()) {
      //Not found
      requests.push_back(p);
    }
  }
  auto result = async_task<>::empty();
  if (!requests.empty()) {

    orm::transaction_log_unknown_record_dbo t1;
    for(const auto & p : requests){
      t.execute(
          t1.insert_or_ignore(
              t1.id = base64::from_bytes(p),
              t1.refer_id = message.source_node(),
              t1.follower_id = std::string()));
    }
  }
  else {
    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(
      t2.select(t2.id)
      .where(t2.state == (int)orm::transaction_log_record_dbo::state_t::leaf));

    std::list<const_data_buffer> current_state;
    while (st.execute()) {
      auto id = base64::to_bytes(t2.id.get(st));
      
      auto exist = false;
      for (auto & p : message.leafs()) {
        if (id == p) {
          exist = true;
          break;;
        }
      }

      if(!exist) {
        current_state.push_back(id);
      }
    }

    if(!current_state.empty()) {
      result = result.then([sp, message, current_state]() {
        auto & client = *sp.get<vds::dht::network::client>();
        client->send_neighbors(
          sp,
          messages::transaction_log_state(
            current_state,
            client->current_node_id()));
      });
    }
  }

  return result;
}

void vds::transaction_log::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const dht::messages::transaction_log_request& message) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  auto st = t.get_reader(
      t1.select(t1.data)
          .where(t1.id == base64::from_bytes(message.transaction_id())));
  if (st.execute()) {

    sp.get<logger>()->trace(
        ThisModule,
        sp,
        "Provide log record %s",
        base64::from_bytes(message.transaction_id()).c_str());

    mt_service::async(sp, [sp, message, data = t1.data.get(st)]() {
      auto &client = *sp.get<vds::dht::network::client>();
      client->send(
          sp,
          message.source_node(),
          messages::transaction_log_record(
              message.transaction_id(),
              data));
    });
  }
}

void vds::transaction_log::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const dht::messages::transaction_log_record& message) {

  sp.get<logger>()->trace(
    ThisModule,
    sp,
    "Save log record %s",
    base64::from_bytes(message.record_id()).c_str());

  transactions::transaction_log::save(
    sp,
    t,
    message.record_id(),
    message.data());
}

void vds::transaction_log::sync_process::sync_local_channels(
  const service_provider& sp,
  database_transaction& t) {
  auto & client = *sp.get<dht::network::client>();

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.id)
    .where(t1.state == static_cast<int>(orm::transaction_log_record_dbo::state_t::leaf)));

  std::list<const_data_buffer> leafs;
  while (st.execute()) {
    leafs.push_back(base64::to_bytes(t1.id.get(st)));
  }

  if (leafs.empty()) {
    return;
  }

  std::string log_message;
  for (const auto & r : leafs) {
    log_message += base64::from_bytes(r);
    log_message += ' ';
  }

  sp.get<logger>()->trace(
    ThisModule,
    sp,
    "state is %s",
    log_message.c_str());

  sp.get<logger>()->trace(ThisModule, sp, "Send transaction_log_state");
  client->send_neighbors(
    sp,
    messages::transaction_log_state(
      leafs,
      client->current_node_id()));

}

void vds::transaction_log::sync_process::sync_replicas(
  const vds::service_provider &sp,
  vds::database_transaction &t) {
  auto & client = *sp.get<dht::network::client>();

  std::set<const_data_buffer /*object_id*/> objects;
  orm::chunk_dbo t1;
  auto st = t.get_reader(
    t1
    .select(t1.object_id)
    .where(t1.last_sync <= std::chrono::system_clock::now() - std::chrono::minutes(10))
    .order_by(t1.last_sync));
  while (st.execute()) {
    const auto replica_hash = base64::to_bytes(t1.object_id.get(st));
    objects.emplace(replica_hash);
  }

  //Move replicas closer to root
  for (const auto & p : objects) {
    std::set<uint16_t> replicas;
    orm::sync_replica_map_dbo t3;
    st = t.get_reader(t3.select(t3.replica).where(t3.object_id == base64::from_bytes(p)));
    while (st.execute()) {
      if (replicas.end() == replicas.find(t3.replica.get(st))) {
        replicas.emplace(t3.replica.get(st));
      }
    }

    if (_client::GENERATE_DISTRIBUTED_PIECES <= replicas.size()) {
      t.execute(t1.update(t1.last_sync = std::chrono::system_clock::now()).where(t1.object_id == base64::from_bytes(p)));
    }

    std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> neighbors;
    client->neighbors(sp, p, neighbors, _client::GENERATE_DISTRIBUTED_PIECES);

    for (auto & pneighbor : neighbors) {
      for (auto & node : pneighbor.second) {
        sp.get<logger>()->trace(ThisModule, sp, "Send offer_replica");
        client->send(sp, node, messages::offer_replica(
          p,
          client->current_node_id()));
      }

    }
  }
}
