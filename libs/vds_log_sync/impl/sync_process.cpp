/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "sync_process.h"
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
#include "../../vds_dht_network/private/dht_network_client_p.h"
#include "messages/transaction_log_state.h"
#include "messages/transaction_log_request.h"
#include "messages/transaction_log_record.h"
#include "messages/offer_replica.h"

void vds::transaction_log::sync_process::do_sync(const service_provider& sp, database_transaction& t) {
  this->sync_local_channels(sp, t);
  this->query_unknown_records(sp, t);
}

void vds::transaction_log::sync_process::query_unknown_records(const service_provider& sp, database_transaction& t) {
  std::set<const_data_buffer> record_ids;
  orm::transaction_log_unknown_record_dbo t1;
  orm::transaction_log_unknown_record_dbo t2;
  auto st = t.get_reader(
    t1.select(
      t1.id)
    .where(db_not_in(t1.id, t2.select(t2.follower_id))));

  while (st.execute()) {
    if (record_ids.end() == record_ids.find(t1.id.get(st))) {
      record_ids.emplace(t1.id.get(st));
    }
  }

  if(record_ids.empty()){
    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(
        t2.select(t2.id)
            .where(t2.state == orm::transaction_log_record_dbo::state_t::leaf));

    std::list<const_data_buffer> current_state;
    while (st.execute()) {
      auto id = t2.id.get(st);
      current_state.push_back(id);
    }

    if(!current_state.empty()) {
        auto client = sp.get<vds::dht::network::client>();
        (*client)->send_neighbors(
          sp,
          dht::messages::transaction_log_state(
                current_state,
                client->current_node_id()));
    }
  }
  else {
    auto &client = *sp.get<dht::network::client>();
    for (const auto & p : record_ids) {
      sp.get<logger>()->trace(
          ThisModule,
          sp,
          "Query log records %s",
          base64::from_bytes(p).c_str());

      std::list<std::shared_ptr<dht::dht_route<std::shared_ptr<dht::network::dht_session>>::node>> neighbors;
      client->get_neighbors(sp, neighbors);

      for (const auto& neighbor : neighbors) {
        client->send(
          sp,
          neighbor->node_id_,
          dht::messages::transaction_log_request(
            p,
            client->current_node_id()));
      }
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
      .where(t1.id == p));
    if (!st.execute()) {
      //Not found
      requests.push_back(p);
    }
  }
  auto result = async_task<>::empty();
  if (!requests.empty()) {

    orm::transaction_log_unknown_record_dbo t3;
    for(const auto & p : requests){
      result = result.then([sp, message, target_node = message.source_node(), transaction_id = p]() {
        auto & client = *sp.get<vds::dht::network::client>();
        client->send(
          sp,
          target_node,
          dht::messages::transaction_log_request(
            transaction_id,
            client->current_node_id()));
      });
    }
  }
  else {
    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(
      t2.select(t2.id)
      .where(t2.state == orm::transaction_log_record_dbo::state_t::leaf));

    std::list<const_data_buffer> current_state;
    while (st.execute()) {
      auto id = t2.id.get(st);
      
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
          dht::messages::transaction_log_state(
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
          .where(t1.id == message.transaction_id()));
  if (st.execute()) {

    sp.get<logger>()->trace(
        ThisModule,
        sp,
        "Provide log record %s",
        base64::from_bytes(message.transaction_id()).c_str());

    mt_service::async(sp, [sp, message, data = t1.data.get(st)]() {
      auto client = sp.get<vds::dht::network::client>();
      (*client)->send(
          sp,
          message.source_node(),
          dht::messages::transaction_log_record(
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
    .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf));

  std::list<const_data_buffer> leafs;
  while (st.execute()) {
    leafs.push_back(t1.id.get(st));
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
    dht::messages::transaction_log_state(
      leafs,
      client->current_node_id()));

}
