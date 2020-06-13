/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/sync_process.h"
#include "dht_network_client.h"
#include "messages/transaction_log_messages.h"
#include "messages/sync_messages.h"
#include "transaction_log_record_dbo.h"
#include "private/dht_network_client_p.h"
#include "chunk_replica_data_dbo.h"
#include "sync_replica_map_dbo.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "sync_message_dbo.h"
#include "db_model.h"
#include "device_config_dbo.h"
#include "vds_exceptions.h"
#include "sync_local_queue_dbo.h"
#include "chunk_dbo.h"
#include "device_record_dbo.h"
#include "dht_network.h"
#include "current_config_dbo.h"
#include "node_info_dbo.h"
#include <local_data_dbo.h>
#include "node_storage_dbo.h"
#include <transaction_block_builder.h>

vds::dht::network::sync_process::sync_process(const service_provider * sp)
  : sp_(sp), sync_replicas_timeout_(0) {
  for (uint16_t replica = 0; replica < service::GENERATE_HORCRUX; ++replica) {
    this->distributed_generators_[replica].reset(new chunk_generator<uint16_t>(service::MIN_HORCRUX, replica));
  }
}

vds::expected<void> vds::dht::network::sync_process::do_sync(  
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {

  CHECK_EXPECTED(this->sync_replicas(t, final_tasks));

  return expected<void>();
}

vds::expected<vds::const_data_buffer> vds::dht::network::sync_process::restore_replica(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer & replica_hash) {

  auto client = this->sp_->get<network::client>();

  std::set<const_data_buffer> candidates;
  orm::sync_replica_map_dbo t5;
  GET_EXPECTED(st, t.get_reader(t5.select(t5.node).where(t5.replica_hash == replica_hash)));
  WHILE_EXPECTED(st.execute()) {
    if (candidates.end() == candidates.find(t5.node.get(st)) && client->current_node_id() != t5.node.get(st)) {
      candidates.emplace(t5.node.get(st));
    }
  }
  WHILE_EXPECTED_END()

  if (!candidates.empty()) {
    for (const auto& candidate : candidates) {
      this->sp_->get<logger>()->trace(
        SyncModule,
        "request replica %s from %s",
        base64::from_bytes(replica_hash).c_str(),
        base64::from_bytes(candidate).c_str());

      final_tasks.push_back([client, candidate, replica_hash]() {
        return (*client)->send(
          candidate,
          message_create<messages::sync_replica_request>(
            replica_hash));
      });
    }
  }
  else {
    final_tasks.push_back([client, replica_hash]() {
      return (*client)->send_near(
        replica_hash,
        1,
        message_create<messages::sync_replica_request>(
          replica_hash),
        [](const dht::dht_route::node& node) -> bool {
          return node.hops_ == 0;
        });
    });
  }

  return const_data_buffer();
}

vds::expected<bool> vds::dht::network::sync_process::prepare_restore_replica(
  database_read_transaction & t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const const_data_buffer replica_hash) {

  auto client = this->sp_->get<network::client>();
  std::list<uint16_t> replicas;

  orm::local_data_dbo t1;
  GET_EXPECTED(st, t.get_reader(
    t1.select(t1.replica_hash)
    .where(t1.replica_hash == replica_hash)));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    return expected<bool>(true);
  }

  std::set<const_data_buffer> candidates;
  orm::sync_replica_map_dbo t5;
  GET_EXPECTED_VALUE(st, t.get_reader(t5.select(t5.node).where(t5.replica_hash == replica_hash)));
  WHILE_EXPECTED(st.execute()) {
    if (candidates.end() == candidates.find(t5.node.get(st)) && client->current_node_id() != t5.node.get(st)) {
      candidates.emplace(t5.node.get(st));
    }
  }
  WHILE_EXPECTED_END()

  for (const auto& candidate : candidates) {
    this->sp_->get<logger>()->trace(
      SyncModule,
      "request %s from %s",
      base64::from_bytes(replica_hash).c_str(),
      base64::from_bytes(candidate).c_str());
    final_tasks.push_back([client, candidate, replica_hash]() {
      return (*client)->send(
        candidate,
        message_create<messages::sync_replica_request>(
          replica_hash));
    });
  }

  return false;
}

vds::expected<bool> vds::dht::network::sync_process::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>>& final_tasks,
  const messages::sync_replica_request& message,
  const imessage_map::message_info_t& message_info) {

  auto client = this->sp_->get<network::client>();
  this->sp_->get<logger>()->trace(
    SyncModule,
    "%s: replica %s request from %s",
    base64::from_bytes(client->current_node_id()).c_str(),
    base64::from_bytes(message.object_id).c_str(),
    base64::from_bytes(message_info.source_node()).c_str());


  orm::local_data_dbo t3;
  orm::node_storage_dbo t4;
  orm::chunk_replica_data_dbo t5;
  GET_EXPECTED(st, t.get_reader(
    t3
    .select(t3.storage_path, t4.local_path, t3.owner, t5.object_hash, t5.replica)
    .inner_join(t4, t4.storage_id == t3.storage_id)
    .inner_join(t5, t5.replica_hash == t3.replica_hash)
    .where(t3.replica_hash == message.object_id)));

  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    this->sp_->get<logger>()->trace(
      SyncModule,
      "Send replica %s to %s",
      base64::from_bytes(message.object_id).c_str(),
      base64::from_bytes(message_info.source_node()).c_str());
    const auto owner = t3.owner.get(st);
    const auto object_hash = t5.object_hash.get(st);
    const auto replica = t5.replica.get(st);
    GET_EXPECTED(data, file::read_all(filename(foldername(t4.local_path.get(st)), t3.storage_path.get(st))));
    final_tasks.push_back([
      client,
        data,
        target_node = message_info.source_node(),
        object_id = message.object_id,
        owner,
        object_hash,
        replica
    ]() {
        return (*client)->send(
          target_node,
          message_create<messages::sync_replica_data>(
            object_id,
            data,
            owner,
            object_hash,
            replica));
      });
  }

  return true;
}

vds::expected<bool> vds::dht::network::sync_process::apply_message(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks,
  const messages::sync_replica_data& message,
  const imessage_map::message_info_t& message_info) {

  auto client = this->sp_->get<network::client>();

  orm::local_data_dbo t2;
  GET_EXPECTED(st, t.get_reader(
    t2.select(t2.replica_hash)
      .where(t2.replica_hash == message.object_id)));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    //Already exists
    this->sp_->get<logger>()->trace(
      SyncModule,
      "Replica %s from %s is already exists",
      base64::from_bytes(message.object_id).c_str(),
      base64::from_bytes(message_info.source_node()).c_str());
    return false;
  }

    GET_EXPECTED(data_hash, hash::signature(hash::sha256(), message.data));
    vds_assert(data_hash == message.object_id);
    GET_EXPECTED(fn, _client::save_data(this->sp_, t, data_hash, message.data, message.owner, message.value_id, message.replica));
    this->sp_->get<logger>()->trace(
      SyncModule,
      "Got replica %s from %s",
      base64::from_bytes(message.object_id).c_str(),
      base64::from_bytes(message_info.source_node()).c_str());

  return true;
}

vds::expected<void> vds::dht::network::sync_process::sync_replicas(
  database_transaction& t,
  std::list<std::function<async_task<expected<void>>()>> & final_tasks) {
  if (this->sync_replicas_timeout_++ == 120) {
    const auto pclient = this->sp_->get<client>();
    std::set<const_data_buffer> requested_objects;
    std::map<const_data_buffer/*object_id*/, std::map<const_data_buffer/*node*/, std::set<uint16_t/*replicas*/>>> nodes;

      orm::chunk_replica_data_dbo t1;
      GET_EXPECTED(st, t.get_reader(
        t1
        .select(t1.owner_id, t1.object_hash, t1.replica, t1.replica_hash, t1.replica_size)
        .order_by(t1.distance)));
      WHILE_EXPECTED(st.execute()) {
        const auto owner = t1.owner_id.get(st);
        const auto object_hash = t1.object_hash.get(st);
        const auto replica = t1.replica.get(st);
        const auto replica_hash = t1.replica_hash.get(st);
        const auto replica_size = t1.replica_size.get(st);

        auto p = nodes.find(object_hash);
        if (nodes.end() == p) {
          std::map<const_data_buffer/*node*/, std::set<uint16_t/*replicas*/>> replica_map;

          orm::sync_replica_map_dbo t2;
          orm::chunk_replica_data_dbo t3;
          GET_EXPECTED(st_map, t.get_reader(
            t3
            .select(t3.replica, t2.node)
            .inner_join(t2, t2.replica_hash == t3.replica_hash)
            .where(t3.object_hash == object_hash)));
          WHILE_EXPECTED(st_map.execute()) {
            const auto replica = t3.replica.get(st_map);
            const auto node = t2.node.get(st_map);

            replica_map[node].emplace(replica);
          }
          WHILE_EXPECTED_END()

          p = nodes.emplace(object_hash, std::move(replica_map)).first;
        }

        std::set<const_data_buffer> stored;
        for (const auto& n : p->second) {
          if (n.second.end() != n.second.find(replica)) {
            stored.emplace(n.first);
          }
        }

        if (stored.empty() || (p->second.size() < service::GENERATE_HORCRUX && stored.end() == stored.find(pclient->current_node_id()))) {
          orm::node_storage_dbo t4;
          orm::local_data_dbo t5;
          GET_EXPECTED(st, t.get_reader(
            t4
            .select(t4.local_path, t5.storage_path)
            .inner_join(t5, t5.storage_id == t4.storage_id)
            .where(t5.replica_hash == replica_hash)));
          GET_EXPECTED(st_execute, st.execute());
          if (st_execute) {
            transactions::transaction_block_builder playload;
            CHECK_EXPECTED(playload.add(message_create<transactions::host_block_transaction>(replica_hash)));
            CHECK_EXPECTED(pclient->save(this->sp_, playload, t, false));
          }
          else if (!stored.empty()) {
            for (const auto& candidate : stored) {
              this->sp_->get<logger>()->trace(
                SyncModule,
                "request %s from %s",
                base64::from_bytes(replica_hash).c_str(),
                base64::from_bytes(candidate).c_str());
              final_tasks.push_back([pclient, candidate, replica_hash]() {
                return (*pclient)->send(
                  candidate,
                  message_create<messages::sync_replica_request>(
                    replica_hash));
                });
            }
          } else if(requested_objects.end() == requested_objects.find(object_hash)) {
            requested_objects.emplace(object_hash);

            std::vector<const_data_buffer> replicas;
            orm::chunk_replica_data_dbo t1;
            GET_EXPECTED(st, t.get_reader(
              t1
              .select(t1.replica, t1.replica_hash)
              .where(t1.object_hash == object_hash)
              .order_by(t1.replica)));
            WHILE_EXPECTED(st.execute()) {
              const auto replica = t1.replica.get(st);
              const auto replica_hash = t1.replica_hash.get(st);
              if (replicas.size() != replica) {
                break;
              }
              replicas.push_back(replica_hash);
            }
            WHILE_EXPECTED_END()

            auto result = std::make_shared<const_data_buffer>();
            CHECK_EXPECTED((*pclient)->restore_async(t, final_tasks, replicas, result, std::make_shared<uint8_t>()));
            
            if (0 < result->size()) {
              CHECK_EXPECTED((*pclient)->save_data(this->sp_, t, *result, owner));
            }
          }
        }

      }
      WHILE_EXPECTED_END()
  }

  return expected<void>();
}

