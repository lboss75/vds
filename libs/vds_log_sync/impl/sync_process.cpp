/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "sync_process.h"
#include "dht_network_client.h"
#include "transaction_log_record_dbo.h"
#include "chunk_dbo.h"
#include "transaction_log_hierarchy_dbo.h"
#include "transaction_log.h"
#include "db_model.h"
#include "messages/transaction_log_messages.h"
#include "../../vds_dht_network/private/dht_network_client_p.h"
#include "transaction_block.h"
#include "node_info_dbo.h"

vds::expected<void> vds::transaction_log::sync_process::do_sync(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {
  CHECK_EXPECTED(this->query_unknown_records(t, final_tasks));
  return expected<void>();
}

vds::expected<void> vds::transaction_log::sync_process::query_unknown_records(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {
  std::set<const_data_buffer> record_ids;
  orm::transaction_log_hierarchy_dbo t1;
  orm::transaction_log_hierarchy_dbo t2;
  orm::transaction_log_record_dbo t3;
  GET_EXPECTED(st, t.get_reader(
    t1.select(
      t1.id)
    .where(db_not_in(t1.id, t2.select(t2.follower_id)) && db_not_in(t1.id, t3.select(t3.id)))));

  WHILE_EXPECTED(st.execute()) {
    if (record_ids.end() == record_ids.find(t1.id.get(st))) {
      record_ids.emplace(t1.id.get(st));
    }
  }
  WHILE_EXPECTED_END()

  if(record_ids.empty()){
    GET_EXPECTED_VALUE(st, t.get_reader(
        t3.select(t3.id)
            .where(t3.state == orm::transaction_log_record_dbo::state_t::leaf)));

    std::list<const_data_buffer> current_state;
    WHILE_EXPECTED(st.execute()) {
      auto id = t3.id.get(st);
      current_state.push_back(id);
    }
    WHILE_EXPECTED_END()

    if(!current_state.empty()) {
        auto client = this->sp_->get<vds::dht::network::client>();
        final_tasks.push_back([current_state, client]() -> async_task<expected<void>>{
        return (*client)->send_neighbors(
          message_create<dht::messages::transaction_log_state>(
            current_state,
            client->current_node_id()));
      });
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
        final_tasks.push_back([node_id = neighbor->node_id_, record_id = p, client]()->async_task<expected<void>> {
          return client->send(
            node_id,
            message_create<dht::messages::transaction_log_request>(
              record_id,
              client->current_node_id()));
        });
      }
    }
  }

  return expected<void>();
}


vds::expected<bool> vds::transaction_log::sync_process::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const dht::messages::transaction_log_state& message,
  const dht::network::imessage_map::message_info_t & message_info) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  for (auto & p : message.leafs) {
    GET_EXPECTED(st, t.get_reader(
      t1.select(t1.state)
      .where(t1.id == p)));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute) {
      //Not found
      requests.push_back(p);
    }
  }
  if (!requests.empty()) {
    for (const auto & p : requests) {
      auto client = this->sp_->get<vds::dht::network::client>();
      final_tasks.push_back([client, source_node = message.source_node, request = p]()->async_task<expected<void>> {
        return (*client)->send(
          source_node,
          message_create<dht::messages::transaction_log_request>(
            request,
            client->current_node_id()));
      });
    }
  }
  else {
    orm::transaction_log_record_dbo t2;
    GET_EXPECTED(st, t.get_reader(
      t2.select(t2.id)
      .where(t2.state == orm::transaction_log_record_dbo::state_t::leaf)));

    std::list<const_data_buffer> current_state;
    WHILE_EXPECTED(st.execute()) {
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
    WHILE_EXPECTED_END()

    if (!current_state.empty()) {
      auto & client = *this->sp_->get<vds::dht::network::client>();
      final_tasks.push_back([client, source_node = message.source_node, current_state]()->async_task<expected<void>> {
        return client->send(
          source_node,
          message_create<dht::messages::transaction_log_state>(
            current_state,
            client->current_node_id()));
      });
    }
  }

  return true;
}

vds::expected<bool> vds::transaction_log::sync_process::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const dht::messages::transaction_log_request& message,
  const dht::network::imessage_map::message_info_t & message_info) {

  orm::transaction_log_record_dbo t1;
  std::list<const_data_buffer> requests;
  GET_EXPECTED(st, t.get_reader(
    t1.select(t1.data)
    .where(t1.id == message.transaction_id)));
  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return false;
  }

  this->sp_->get<logger>()->trace(
    ThisModule,
    "Provide log record %s",
    base64::from_bytes(message.transaction_id).c_str());

  auto client = this->sp_->get<vds::dht::network::client>();
  final_tasks.push_back([client, data = t1.data.get(st), source_node = message.source_node, transaction_id = message.transaction_id]()->async_task<expected<void>> {
    return (*client)->send(
      source_node,
      message_create<dht::messages::transaction_log_record>(
        transaction_id,
        data));
  });

  return true;
}

vds::expected<bool> vds::transaction_log::sync_process::apply_message(
    database_transaction& t,
    std::list<std::function<async_task<expected<void>>()>> & final_tasks,
    const dht::messages::transaction_log_record& message,
    const dht::network::imessage_map::message_info_t & message_info) {

  this->sp_->get<logger>()->trace(
    ThisModule,
    "Save log record %s",
    base64::from_bytes(message.record_id).c_str());

  GET_EXPECTED(block, transactions::transaction_block::create(message.data));
  GET_EXPECTED(block_exists, block.exists(t));
  if (block_exists) {
    return false;
  }

  orm::transaction_log_record_dbo t1;
  std::shared_ptr<asymmetric_public_key> write_public_key;
  orm::node_info_dbo t2;
  GET_EXPECTED(st, t.get_reader(t2.select(t2.public_key).where(t2.node_id == block.write_public_key_id())));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    GET_EXPECTED(write_public_key_data, asymmetric_public_key::parse_der(t2.public_key.get(st)));
    write_public_key = std::make_shared<asymmetric_public_key>(std::move(write_public_key_data));
  }

  if (!write_public_key) {
    GET_EXPECTED(block, transactions::transaction_block::create(message.data));
    CHECK_EXPECTED(block.walk_messages([&block, &write_public_key](const transactions::node_add_transaction & message)->expected<bool> {
      GET_EXPECTED(node_id, message.node_public_key->hash(hash::sha256()));
      if (block.write_public_key_id() == node_id) {
        write_public_key = message.node_public_key;
        return false;
      }

      return true;
    }));
  }

  if (!write_public_key || !block.validate(*write_public_key)) {
    auto client = this->sp_->get<vds::dht::network::client>();

    orm::transaction_log_hierarchy_dbo t4;
    for (const auto & ancestor : block.ancestors()) {
      GET_EXPECTED(st, t.get_reader(t1.select(t1.state).where(t1.id == ancestor)));
      GET_EXPECTED(st_execute, st.execute());
      if (!st_execute) {
        final_tasks.push_back([client, target = ancestor, source_node = message_info.source_node()]() {
          return (*client)->send(
            source_node,
            message_create<dht::messages::transaction_log_request>(
              target,
              client->current_node_id()));
        });
      }
    }

    return false;
  }

  CHECK_EXPECTED(transactions::transaction_log::save(
    this->sp_,
    t,
    message.data));

  return true;
}

vds::expected<void> vds::transaction_log::sync_process::on_new_session(
  database_read_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer& partner_id) {
  return this->sync_local_channels(t, final_tasks, partner_id);
}

vds::expected<void> vds::transaction_log::sync_process::sync_local_channels(
  database_read_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer& partner_id) {

  auto client = this->sp_->get<dht::network::client>();

  orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st, t.get_reader(
    t1.select(t1.id)
    .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf)));

  std::list<const_data_buffer> leafs;
  WHILE_EXPECTED(st.execute()) {
    leafs.push_back(t1.id.get(st));
  }
  WHILE_EXPECTED_END()

  if (leafs.empty()) {
    return expected<void>();
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
  final_tasks.push_back([client, leafs, partner_id]() {
    return (*client)->send(
      partner_id,
      message_create<dht::messages::transaction_log_state>(
        leafs,
        client->current_node_id()));
  });

  return expected<void>();
}
