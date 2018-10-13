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
#include "db_model.h"
#include "messages/transaction_log_messages.h"
#include "../../vds_dht_network/private/dht_network_client_p.h"

vds::async_task<void> vds::transaction_log::sync_process::do_sync( database_transaction& t) {
  co_await this->sync_local_channels(t, const_data_buffer());
  co_await this->query_unknown_records(t);
}

vds::async_task<void> vds::transaction_log::sync_process::query_unknown_records( database_transaction& t) {
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
        auto client = this->sp_->get<vds::dht::network::client>();
        co_await (*client)->send_neighbors(
          message_create<dht::messages::transaction_log_state>(
                current_state,
                client->current_node_id()));
    }
  }
  else {
    auto &client = *this->sp_->get<dht::network::client>();
    for (const auto & p : record_ids) {
      this->sp_->get<logger>()->trace(
          ThisModule,
          "Query log records %s",
          base64::from_bytes(p).c_str());

      std::list<std::shared_ptr<dht::dht_route<std::shared_ptr<dht::network::dht_session>>::node>> neighbors;
      client->get_neighbors(neighbors);

      for (const auto& neighbor : neighbors) {
        co_await client->send(
          neighbor->node_id_,
          message_create<dht::messages::transaction_log_request>(
            p,
            client->current_node_id()));
      }
    }
  }
}


vds::async_task<void> vds::transaction_log::sync_process::apply_message(
  
  database_transaction& t,
  const dht::messages::transaction_log_state& message,
  const dht::network::imessage_map::message_info_t & message_info) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  for (auto & p : message.leafs) {
    auto st = t.get_reader(
      t1.select(t1.state)
      .where(t1.id == p));
    if (!st.execute()) {
      //Not found
      requests.push_back(p);
    }
  }
  if (!requests.empty()) {
    orm::transaction_log_unknown_record_dbo t3;
    for (const auto & p : requests) {
      auto & client = *this->sp_->get<vds::dht::network::client>();
      co_await client->send(
        message.source_node,
        message_create<dht::messages::transaction_log_request>(
          p,
          client->current_node_id()));
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
      for (auto & p : message.leafs) {
        if (id == p) {
          exist = true;
          break;;
        }
      }

      if (!exist) {
        current_state.push_back(id);
      }
    }

    if (!current_state.empty()) {
      auto & client = *this->sp_->get<vds::dht::network::client>();
      co_await client->send_neighbors(
        message_create<dht::messages::transaction_log_state>(
          current_state,
          client->current_node_id()));
    }
  }
}

vds::async_task<void> vds::transaction_log::sync_process::apply_message(

  database_transaction& t,
  const dht::messages::transaction_log_request& message,
  const dht::network::imessage_map::message_info_t & message_info) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  auto st = t.get_reader(
    t1.select(t1.data)
    .where(t1.id == message.transaction_id));
  if (st.execute()) {

    this->sp_->get<logger>()->trace(
      ThisModule,
      "Provide log record %s",
      base64::from_bytes(message.transaction_id).c_str());

    auto client = this->sp_->get<vds::dht::network::client>();
    co_await(*client)->send(
      message.source_node,
      message_create<dht::messages::transaction_log_record>(
        message.transaction_id,
        t1.data.get(st)));

  }
}

vds::async_task<void> vds::transaction_log::sync_process::apply_message(
    
    database_transaction& t,
    const dht::messages::transaction_log_record& message,
    const dht::network::imessage_map::message_info_t & message_info) {

  this->sp_->get<logger>()->trace(
    ThisModule,
    "Save log record %s",
    base64::from_bytes(message.record_id).c_str());

  if(transactions::transaction_log::save(
    this->sp_,
    t,
    message.data)) {
    co_await this->sync_local_channels(t, const_data_buffer());
  }
}

vds::async_task<void> vds::transaction_log::sync_process::on_new_session(
  
  database_read_transaction & t,
  const const_data_buffer& partner_id) {
  return this->sync_local_channels(t, partner_id);
}

vds::async_task<void> vds::transaction_log::sync_process::sync_local_channels(
  
  database_read_transaction& t,
  const const_data_buffer& partner_id) {

  auto & client = *this->sp_->get<dht::network::client>();

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.id)
    .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf));

  std::list<const_data_buffer> leafs;
  while (st.execute()) {
    leafs.push_back(t1.id.get(st));
  }

  if (leafs.empty()) {
    co_return;
  }

  std::string log_message;
  for (const auto & r : leafs) {
    log_message += base64::from_bytes(r);
    log_message += ' ';
  }

  this->sp_->get<logger>()->trace(
    ThisModule,
    "state is %s",
    log_message.c_str());

  this->sp_->get<logger>()->trace(ThisModule, "Send transaction_log_state");
  if (!partner_id) {
    co_await client->send_neighbors(
      message_create<dht::messages::transaction_log_state>(
        leafs,
        client->current_node_id()));
  }
  else {
    co_await client->send(
      partner_id,
      message_create<dht::messages::transaction_log_state>(
        leafs,
        client->current_node_id()));
  }
}
