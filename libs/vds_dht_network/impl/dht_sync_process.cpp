/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_sync_process.h"
#include "dht_network_client.h"
#include "messages/transaction_log_state.h"
#include "transaction_log_record_dbo.h"
#include "chunk_replicas_dbo.h"
#include "private/dht_network_client_p.h"
#include "transaction_log_unknown_record_dbo.h"
#include "messages/transaction_log_request.h"
#include "messages/transaction_log_record.h"
#include "messages/offer_replica.h"
#include "transaction_log.h"

vds::async_task<> vds::dht::network::sync_process::query_unknown_records(const service_provider& sp, database_transaction& t) {
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

  auto result = async_task<>::empty();
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
      result = result.then([sp, current_state]() {
        auto & client = *sp.get<vds::dht::network::client>();
        client->send_neighbors(
            sp,
            messages::transaction_log_state(
                current_state,
                client->current_node_id())).no_wait();
      });
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

      result = result.then([client, sp, node_id = p.second[index], record_id = p.first]() {
        client->send(
            sp,
            node_id,
            messages::transaction_log_request(
                record_id,
                client->current_node_id())).no_wait();
      });
    }
  }
  return result;
}

vds::async_task<> vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {
  return async_series(
    this->sync_local_channels(sp, t),
    this->query_unknown_records(sp, t),
    this->sync_replicas(sp, t)
  );
}

vds::async_task<> vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::transaction_log_state& message) {

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
        client->send(
          sp,
          message.source_node(),
          messages::transaction_log_state(
            current_state,
            client->current_node_id())).no_wait();
      });
    }
  }

  return result;
}

vds::async_task<> vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::transaction_log_request& message) {

  auto result = async_task<>::empty();
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

    result = result.then([
                             sp,
                             message,
                             data = t1.data.get(st)]() {
      auto &client = *sp.get<vds::dht::network::client>();
      return client->send(
          sp,
          message.source_node(),
          messages::transaction_log_record(
              message.transaction_id(),
              data));
    });
  }

  return result;
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::transaction_log_record& message) {

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

vds::async_task<> vds::dht::network::sync_process::sync_local_channels(const service_provider& sp, database_transaction& t) {
  auto & client = *sp.get<dht::network::client>();

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.id)
    .where(t1.state == static_cast<int>(orm::transaction_log_record_dbo::state_t::leaf)));

  std::list<const_data_buffer> leafs;
  while (st.execute()) {
    leafs.push_back(base64::to_bytes(t1.id.get(st)));
  }

  if(leafs.empty()){
    return async_task<>::empty();
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

  return client->send_neighbors(
    sp,
    messages::transaction_log_state(
      leafs,
      client->current_node_id()));

}

vds::async_task<> vds::dht::network::sync_process::sync_replicas(
    const vds::service_provider &sp,
    vds::database_transaction &t) {
  auto & client = *sp.get<dht::network::client>();

  std::map<const_data_buffer /*replica_hash*/, std::tuple<vds::const_data_buffer /*distance*/, vds::const_data_buffer /*data*/>> objects;
  orm::chunk_replicas_dbo t1;
  auto st = t.get_reader(
    t1
    .select(t1.id, t1.replica_data)
    .where(t1.last_sync <= std::chrono::system_clock::now() - std::chrono::minutes(10))
    .order_by(t1.last_sync));
  while(st.execute()){
    const auto replica_hash = base64::to_bytes(t1.id.get(st));
    objects[replica_hash] = std::make_tuple(dht_object_id::distance(replica_hash, client.current_node_id()), t1.replica_data.get(st));
  }

  //Move replicas closer to root
  auto result = async_task<>::empty();
  for (auto & p : objects) {
    std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> neighbors;
    client->neighbors(sp, p.first, neighbors, _client::GENERATE_HORCRUX);

    for (auto & pneighbor : neighbors) {
      if (pneighbor.first < std::get<0>(p.second)) {
        for (auto & node : pneighbor.second) {
          result = result.then([
            client,
              sp,
              replica_hash = p.first,
              target_node = node,
              replica_data = std::get<1>(p.second)](){
              client->send(sp, target_node, messages::offer_replica(
                replica_hash,
                replica_data,
                client->current_node_id()));
            });
        }
        break;
      }
    }
  }
  return result;
}

void vds::dht::network::sync_process::apply_message(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::dht::messages::offer_replica &message) {

  orm::chunk_replicas_dbo t1;
  t.execute(t1.insert_or_ignore(
      t1.id = base64::from_bytes(message.replica_hash()),
      t1.replica_data = message.replica_data()));

  auto & client = *sp.get<vds::dht::network::client>();
  client->send(sp, message.source_node(), messages::got_replica(
      message.replica_hash(),
      client->current_node_id()));
}

void vds::dht::network::sync_process::apply_message(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::dht::messages::got_replica &message) {
  //TODO: validate operation
  orm::chunk_replicas_dbo t1;
  t.execute(t1.delete_if(
      t1.id == base64::from_bytes(message.replica_hash())));

}
