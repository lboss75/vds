/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/sync_process.h"
#include "dht_network_client.h"
#include "messages/transaction_log_state.h"
#include "transaction_log_record_dbo.h"
#include "private/dht_network_client_p.h"
#include "chunk_replica_data_dbo.h"
#include "sync_replica_map_dbo.h"
#include "messages/sync_new_election.h"
#include "messages/sync_coronation.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "messages/sync_leader_broadcast.h"
#include "sync_message_dbo.h"
#include "messages/sync_replica_operations.h"
#include "messages/sync_looking_storage.h"
#include "db_model.h"
#include "device_config_dbo.h"
#include "messages/sync_snapshot.h"
#include "vds_exceptions.h"
#include "sync_local_queue_dbo.h"
#include "messages/sync_add_message.h"
#include "chunk_dbo.h"
#include "messages/sync_replica_request.h"
#include "messages/sync_offer_send_replica_operation.h"
#include "messages/sync_offer_remove_replica_operation.h"
#include "device_record_dbo.h"
#include "messages/sync_replica_data.h"

vds::dht::network::sync_process::sync_process() {
  for (uint16_t replica = 0; replica < GENERATE_DISTRIBUTED_PIECES; ++replica) {
    this->distributed_generators_[replica].reset(new chunk_generator<uint16_t>(MIN_DISTRIBUTED_PIECES, replica));
  }
}

void vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {

  this->sync_entries(sp, t);
  this->sync_local_queues(sp, t);
  this->sync_replicas(sp, t);
}

void vds::dht::network::sync_process::add_to_log(
  const vds::service_provider& sp,
  vds::database_transaction& t,
  const vds::const_data_buffer& object_id,
  vds::orm::sync_message_dbo::message_type_t message_type,
  const vds::const_data_buffer& member_node,
  uint16_t replica,
  const vds::const_data_buffer& source_node,
  uint64_t source_index) {
  orm::sync_message_dbo t3;
  auto st = t.get_reader(t3.select(t3.object_id)
    .where(t3.object_id == object_id
      && t3.source_node == source_node
      && t3.source_index == source_index));
  if(st.execute()) {
    return;
  }

  auto client = sp.get<dht::network::client>();
  orm::sync_member_dbo t2;
  st = t.get_reader(t2.select(
                        t2.generation,
                        t2.current_term,
                        t2.commit_index,
                        t2.last_applied)
                      .where(t2.object_id == object_id
                        && t2.member_node == client->client::current_node_id()));

  if(!st.execute()) {
    throw vds_exceptions::invalid_operation();      
  }

  const auto generation = t2.generation.get(st);
  const auto current_term = t2.current_term.get(st);
  auto commit_index = t2.commit_index.get(st);
  const auto last_applied = t2.last_applied.get(st) + 1;

  t.execute(t3.insert(
    t3.object_id = object_id,
    t3.generation = generation,
    t3.current_term = current_term,
    t3.index = last_applied,
    t3.message_type = message_type,
    t3.member_node = member_node,
    t3.replica = replica,
    t3.source_node = source_node,
    t3.source_index = source_index));

  t.execute(t2.update(
                t2.last_applied = last_applied)
              .where(t2.object_id == object_id
                && t2.member_node == client->client::current_node_id()));

  auto members = get_members(sp, t, object_id);
  for (const auto & member : members) {
    if (client->client::current_node_id() != member) {
      (*client)->send(
        sp,
        member,
        messages::sync_replica_operations_request(
          object_id,
          generation,
          current_term,
          commit_index,
          last_applied,
          message_type,
          member_node,
          replica,
          source_node,
          source_index));
    }
  }
  auto quorum = this->get_quorum(sp, t, object_id);
  sp.get<logger>()->trace(
    SyncModule,
    sp,
    "Sync %s: quorum=%d, commit_index=%d, last_applied=%d",
    base64::from_bytes(object_id).c_str(),
    quorum,
    commit_index,
    last_applied);
  while (quorum < 2 && commit_index < last_applied) {
    this->apply_record(sp, t, object_id, generation, current_term, ++commit_index);
  }
}

void vds::dht::network::sync_process::add_local_log(
  const service_provider& sp,
  database_transaction& t,
  const const_data_buffer& object_id,
  orm::sync_message_dbo::message_type_t message_type,
  const const_data_buffer & member_node,
  uint16_t replica,
  const vds::const_data_buffer& leader_node) {

  orm::sync_local_queue_dbo t1;
  auto st = t.get_reader(t1
    .select(t1.last_send)
    .where(t1.object_id == object_id
      && t1.message_type == message_type
      && t1.member_node == member_node
      && t1.replica == replica));
  if(st.execute()) {
    return;
  }

  t.execute(t1.insert(
    t1.object_id = object_id,
    t1.message_type = message_type,
    t1.member_node = member_node,
    t1.replica = replica,
    t1.last_send = std::chrono::system_clock::now()));

  const auto member_index = t.last_insert_rowid();
  auto client = sp.get<dht::network::client>();
  if(leader_node == client->current_node_id()) {
    this->add_to_log(
      sp,
      t,
      object_id,
      message_type,
      member_node,
      replica,
      client->current_node_id(),
      member_index);
  }
  else {
    (*client)->send(
      sp,
      leader_node,
      messages::sync_add_message_request(
        object_id,
        leader_node,
        client->current_node_id(),
        member_index,
        message_type,
        member_node,
        replica));
  }
}

std::set<vds::const_data_buffer> vds::dht::network::sync_process::get_members(
  const service_provider& sp,
  database_read_transaction& t,
  const vds::const_data_buffer& object_id) {

  orm::sync_member_dbo t1;
  auto st = t.get_reader(t1.select(t1.member_node).where(t1.object_id == object_id));

  std::set<vds::const_data_buffer> result;
  while(st.execute()) {
    result.emplace(t1.member_node.get(st));
  }

  return result;
}

void vds::dht::network::sync_process::make_new_election(
  const service_provider & sp,
  database_transaction & t,
  const const_data_buffer & object_id) const {
  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t2.generation, t2.current_term)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == object_id));

  if (st.execute()) {
    const auto generation = t2.generation.get(st);
    const auto current_term = t2.current_term.get(st);

    t.execute(t1.update(
      t1.state = orm::sync_state_dbo::state_t::canditate,
      t1.next_sync = std::chrono::system_clock::now() + ELECTION_TIMEOUT())
      .where(t1.object_id == object_id));

    t.execute(t2.update(
      t2.voted_for = client->current_node_id(),
      t2.current_term = current_term + 1,
      t2.commit_index = 0,
      t2.last_applied = 0,
      t2.last_activity = std::chrono::system_clock::now())
      .where(t2.object_id == object_id && t2.member_node == client->current_node_id()));

    auto members = this->get_members(sp, t, object_id);
    for (const auto & member : members) {
      client->send(
        sp,
        member,
        messages::sync_new_election_request(
          object_id,
          generation,
          current_term,
          client->current_node_id()));
    }
  }
  else {
    throw vds_exceptions::not_found();
  }
}

vds::dht::network::sync_process::base_message_type vds::dht::network::sync_process::apply_base_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_base_message_request & message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t2.generation, t2.current_term, t2.voted_for, t2.last_applied, t2.commit_index)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));

  if (st.execute()) {
    if (
      message.generation() > t2.generation.get(st)
      || (message.generation() == t2.generation.get(st) && message.current_term() > t2.current_term.get(st))) {
      send_snapshot_request(sp, message.object_id(), message_info.source_node());
      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Object %s from feature. Leader %s",
        base64::from_bytes(message.object_id()).c_str(),
        base64::from_bytes(message_info.source_node()).c_str());
      return base_message_type::from_future;
    }
    else if (
      message.generation() < t2.generation.get(st)
      || (message.generation() == t2.generation.get(st) && message.current_term() < t2.current_term.get(st))) {

      const auto leader = this->get_leader(sp, t, message.object_id());
      if(!leader || client->current_node_id() == leader) {
        send_snapshot(sp, t, message.object_id(), { message_info.source_node() });
      }
      else {
        send_snapshot_request(sp, message.object_id(), message_info.source_node());
      }

      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Object %s from past. Leader %s",
        base64::from_bytes(message.object_id()).c_str(),
        base64::from_bytes(message_info.source_node()).c_str());

      return base_message_type::from_past;
    }
    else if(
      message.generation() == t2.generation.get(st)
      && t2.voted_for.get(st) != message_info.source_node()) {
        this->make_new_election(sp, t, message.object_id());

        sp.get<logger>()->trace(
          SyncModule,
          sp,
          "Object %s other leader. Leader %s",
          base64::from_bytes(message.object_id()).c_str(),
          base64::from_bytes(message_info.source_node()).c_str());
        return base_message_type::other_leader;
    }
    else if(message.generation() == t2.generation.get(st)
      && message.current_term() == t2.current_term.get(st)) {
      return base_message_type::successful;
    }
    else {
      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Object %s case error. Leader %s",
        base64::from_bytes(message.object_id()).c_str(),
        base64::from_bytes(message_info.source_node()).c_str());
      throw std::runtime_error("Case error");
    }
  }
  else {
    sp.get<logger>()->trace(
      SyncModule,
      sp,
      "Object %s not found. Leader %s",
      base64::from_bytes(message.object_id()).c_str(),
      base64::from_bytes(message_info.source_node()).c_str());
    return base_message_type::not_found;
  }
}

uint32_t vds::dht::network::sync_process::get_quorum(
  const service_provider& sp,
  database_read_transaction& t,
  const const_data_buffer& object_id) const {

  db_value<uint64_t> member_count;
  orm::sync_member_dbo t1;
  auto st = t.get_reader(t1.select(
    db_count(t1.member_node).as(member_count))
    .where(t1.object_id == object_id));

  if(st.execute()) {
    return member_count.get(st) / 2 + 1;
  }

  return 0;
}

bool vds::dht::network::sync_process::apply_base_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_base_message_response & message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t2.generation, t2.current_term, t2.voted_for, t2.last_applied, t2.commit_index)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));

  if (st.execute()) {
    if (
      message.generation() > t2.generation.get(st)
      || (message.generation() == t2.generation.get(st) && message.current_term() > t2.current_term.get(st))) {
      vds_assert(false);
      return false;
    }
    else if (
      message.generation() < t2.generation.get(st)
      || (message.generation() == t2.generation.get(st) && message.current_term() < t2.current_term.get(st))) {

      const auto leader = this->get_leader(sp, t, message.object_id());
      if (!leader || client->current_node_id() == leader) {
        send_snapshot(sp, t, message.object_id(), { message_info.source_node() });
      }
      else {
        send_snapshot_request(sp, message.object_id(), leader, message_info.source_node());
      }

      return false;
    }
    else if (message.generation() == t2.generation.get(st)
      && message.current_term() == t2.current_term.get(st)) {

      if(t1.state.get(st) == orm::sync_state_dbo::state_t::leader) {
        const auto generation = t2.generation.get(st);
        const auto current_term = t2.current_term.get(st);
        const auto last_applied = t2.last_applied.get(st);
        auto commit_index = t2.commit_index.get(st);

        st = t.get_reader(t2.select(
          t2.generation,
          t2.current_term,
          t2.commit_index,
          t2.last_applied)
          .where(t2.object_id == message.object_id() && t2.member_node == message_info.source_node()));
        if(st.execute()) {
          if (t2.last_applied.get(st) < message.last_applied()) {
            t.execute(t2.update(
              t2.generation = message.generation(),
              t2.current_term = message.current_term(),
              t2.commit_index = message.commit_index(),
              t2.last_applied = message.last_applied(),
              t2.last_activity = std::chrono::system_clock::now())
              .where(t2.object_id == message.object_id() && t2.member_node == message_info.source_node()));

            for (;;) {
              const auto quorum = this->get_quorum(sp, t, message.object_id());

              db_value<uint64_t> applied_count;
              st = t.get_reader(t2.select(
                db_count(t2.member_node).as(applied_count))
                .where(t2.object_id == message.object_id()
                  && t2.generation == generation
                  && t2.current_term == current_term
                  && t2.last_applied > commit_index));
              
              if(!st.execute()) {
                break;
              }

              if(applied_count.get(st) >= quorum) {
                this->apply_record(sp, t, message.object_id(), generation, current_term, ++commit_index);
              }
              else {
                break;
              }
            }
          }
        }
      }
    }
    else {
      vds_assert(false);
    }
  }
  else {
    vds_assert(false);
    return false;
  }

  return true;
}

void vds::dht::network::sync_process::add_sync_entry(
    const service_provider &sp,
    database_transaction &t,
    const const_data_buffer &object_id,
    uint32_t object_size) {

  const_data_buffer leader;
  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(t1.state, t2.voted_for)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client.current_node_id())
    .where(t1.object_id == object_id));
  if(!st.execute()) {
    leader = client.current_node_id();

    t.execute(t1.insert(
      t1.object_id = object_id,
      t1.object_size = object_size,
      t1.state = orm::sync_state_dbo::state_t::leader,
      t1.next_sync = std::chrono::system_clock::now() + FOLLOWER_TIMEOUT()));

    t.execute(t2.insert(
      t2.object_id = object_id,
      t2.member_node = client->current_node_id(),
      t2.last_activity = std::chrono::system_clock::now(),
      t2.voted_for = client.current_node_id(),
      t2.generation = 0,
      t2.current_term = 0,
      t2.commit_index = 0,
      t2.last_applied = 0));

    client->send_near(
      sp,
      object_id,
      GENERATE_DISTRIBUTED_PIECES,
      messages::sync_looking_storage_request(
        object_id,
        0,
        0,
        0,
        0,
        object_size));
  }
  else {
    leader = t2.voted_for.get(st);
  }

  sp.get<logger>()->trace(SyncModule, sp, "Make leader %s:0:0", base64::from_bytes(object_id).c_str());

  orm::sync_replica_map_dbo t3;
  for(uint16_t i = 0; i < GENERATE_DISTRIBUTED_PIECES; ++i) {
    this->add_local_log(
      sp,
      t,
      object_id,
      orm::sync_message_dbo::message_type_t::add_replica,
      client->current_node_id(),
      i,
      leader);
  }
}

vds::const_data_buffer vds::dht::network::sync_process::restore_replica(
  const service_provider& sp,
  database_transaction& t,
  const const_data_buffer& object_id) {
  auto client = sp.get<dht::network::client>();
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  orm::chunk_replica_data_dbo t2;
  orm::device_record_dbo t4;
  auto st = t.get_reader(
    t2.select(
      t2.replica, t2.replica_hash, t4.local_path)
    .inner_join(t4, t4.node_id == client->current_node_id() && t4.data_hash == t2.replica_hash)
    .where(t2.object_id == object_id));
  while (st.execute()) {
    replicas.push_back(t2.replica.get(st));
    datas.push_back(
      _client::read_data(
        t2.replica_hash.get(st),
        filename(t4.local_path.get(st))));

    if (replicas.size() >= MIN_DISTRIBUTED_PIECES) {
      break;
    }
  }

  if (replicas.size() >= MIN_DISTRIBUTED_PIECES) {
    chunk_restore <uint16_t> restore(MIN_DISTRIBUTED_PIECES, replicas.data());
    binary_serializer s;
    restore.restore(s, datas);

    auto data = s.data();
    const auto data_hash = hash::signature(hash::sha256(), data);
    _client::save_data(sp, t, data_hash, data);

    sp.get<logger>()->trace(SyncModule, sp, "Restored object %s", base64::from_bytes(object_id).c_str());
    orm::chunk_dbo t1;
    t.execute(
      t1.insert(
        t1.object_id = object_id,
        t1.replica_hash = data_hash,
        t1.last_sync = std::chrono::system_clock::now()));

    return data;
  }
  else {
    std::string log_message = "request replica " + base64::from_bytes(object_id) + ". Exists: ";
    std::set<uint16_t> exist_replicas;
    for (auto p : replicas) {
      log_message += std::to_string(p);
      log_message += ',';

      exist_replicas.emplace(p);
    }

    sp.get<logger>()->trace(SyncModule, sp, "%s", log_message.c_str());


    std::set<const_data_buffer> candidates;
    orm::sync_replica_map_dbo t5;
    st = t.get_reader(t5.select(t5.node).where(t5.object_id == object_id));
    while (st.execute()) {
      if (candidates.end() == candidates.find(t5.node.get(st))) {
        candidates.emplace(t5.node.get(st));
      }
    }

    orm::sync_member_dbo t6;
    st = t.get_reader(t6.select(t6.voted_for, t6.member_node).where(t6.object_id == object_id));
    while (st.execute()) {
      if (candidates.end() == candidates.find(t6.member_node.get(st))) {
        candidates.emplace(t6.member_node.get(st));
      }
      if (candidates.end() == candidates.find(t6.voted_for.get(st))) {
        candidates.emplace(t6.voted_for.get(st));
      }
    }

    if (!candidates.empty()) {
      for (const auto & candidate : candidates) {
        sp.get<logger>()->trace(
            SyncModule,
            sp,
            "%s from %s",
            log_message.c_str(),
            base64::from_bytes(candidate).c_str());

        (*client)->send(
          sp,
          candidate,
          messages::sync_replica_request(
            object_id,
            exist_replicas));
      }
    }
    else {
      (*client)->send_near(
        sp,
        object_id,
        GENERATE_DISTRIBUTED_PIECES,
        messages::sync_replica_request(
          object_id,
          exist_replicas));
    }

    return const_data_buffer();
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction & t,
  const messages::sync_looking_storage_request & message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();
  client->send_closer(
    sp,
    message.object_id(),
    GENERATE_DISTRIBUTED_PIECES,
    message);

  switch (this->apply_base_message(sp, t, message, message_info)) {
  case base_message_type::not_found:
  case base_message_type::successful:
    break;

  case base_message_type::from_future:
  case base_message_type::from_past:
  case base_message_type::other_leader:
    return;

  default:
    throw vds_exceptions::invalid_operation();
  }

  for (const auto & record : orm::device_config_dbo::get_free_space(t, client.current_node_id())) {
    if (record.used_size + message.object_size() < record.reserved_size
      && message.object_size() < record.free_size) {

      std::set<uint16_t> replicas;
      orm::chunk_replica_data_dbo t1;
      auto st = t.get_reader(t1.select(t1.replica).where(t1.object_id == message.object_id()));
      while (st.execute()) {
        replicas.emplace(t1.replica.get(st));
      }
      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Ready to store object %s",
        base64::from_bytes(message.object_id()).c_str());

      client->send(
        sp,
        message_info.source_node(),
        messages::sync_looking_storage_response(
          message.object_id(),
          replicas));

      return;
    }
  }
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction & t,
    const messages::sync_looking_storage_response & message,
    const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(
      t1.select(
              t1.state,
              t2.generation,
              t2.current_term,
              t2.commit_index,
              t2.last_applied,
              t2.voted_for)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
          .where(t1.object_id == message.object_id()));

  if(!st.execute()) {
    return;
  }

  if (orm::sync_state_dbo::state_t::leader != static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st))) {
    return;
  }

  const auto generation = t2.generation.get(st);
  const auto current_term = t2.current_term.get(st);
  const auto commit_index = t2.commit_index.get(st);
  auto index = t2.last_applied.get(st);

  st = t.get_reader(t2.select(t2.generation)
      .where(t2.object_id == message.object_id()
             && t2.member_node == message_info.source_node()));
  if(!st.execute()) {
    
    sp.get<logger>()->trace(
      SyncModule,
      sp,
      "Add member %s to store object %s",
      base64::from_bytes(message_info.source_node()).c_str(),
      base64::from_bytes(message.object_id()).c_str());

    this->add_local_log(
      sp,
      t,
      message.object_id(),
      orm::sync_message_dbo::message_type_t::add_member,
      message_info.source_node(),
      0,
      client->current_node_id());
  }

  if (!message.replicas().empty()) {
    //Register replica
    for (auto replica : message.replicas()) {
      this->add_local_log(
        sp,
        t,
        message.object_id(),
        orm::sync_message_dbo::message_type_t::add_replica,
        message_info.source_node(),
        replica,
        client->current_node_id());
    }
  }
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::sync_snapshot_request& message,
    const imessage_map::message_info_t & message_info) {
  auto & client = *sp.get<dht::network::client>();
  const auto leader = this->get_leader(sp, t, message.object_id());
  if (leader && client->current_node_id() != leader) {
    client->send(
      sp,
      leader,
      message);
    return;
  }

  send_snapshot(sp, t, message.object_id(), { message.source_node() });
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_snapshot_response& message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  sp.get<logger>()->trace(
    SyncModule,
    sp,
    "Got snapshot %s form %s",
    base64::from_bytes(message.object_id()).c_str(),
    base64::from_bytes(message.leader_node()).c_str());

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t2.generation, t2.current_term, t2.voted_for, t2.last_applied, t2.commit_index)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));

  if (st.execute()) {
    const auto state = t1.state.get(st);
    const auto generation = t2.generation.get(st);
    const auto current_term = t2.current_term.get(st);
    const auto voted_for = t2.voted_for.get(st);
    const auto last_applied = t2.last_applied.get(st);
    const auto commit_index = t2.commit_index.get(st);

    if (
      message.generation() < generation
      || (message.generation() == generation && message.current_term() < current_term)) {
      if (state == orm::sync_state_dbo::state_t::leader) {
        send_snapshot(sp, t, message.object_id(), { message.leader_node() });
      }
      else {
        send_snapshot_request(sp, message.object_id(), voted_for, message.leader_node());
      }
      return;
    }

    if(last_applied > message.last_applied()) {
      return;
    }

    //merge
    t.execute(t1.update(
      t1.state = orm::sync_state_dbo::state_t::follower,
      t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT())
    .where(t1.object_id == message.object_id()));

    auto members = this->get_members(sp, t, message.object_id());
    for (const auto & member : message.members()) {
      auto p = members.find(member.first);
      if (members.end() == p) {
        t.execute(t2.insert(
          t2.object_id = message.object_id(),
          t2.member_node = member.first,
          t2.voted_for = message.leader_node(),
          t2.generation = message.generation(),
          t2.current_term = message.current_term(),
          t2.commit_index = message.commit_index(),
          t2.last_applied = message.last_applied(),
          t2.last_activity = std::chrono::system_clock::now()));
      }
      else {
        members.erase(p);
      }
    }

    for (const auto & member : members) {
      t.execute(t2.delete_if(
        t2.object_id == message.object_id()
        && t2.member_node == member));
    }
  }
  else if(message.members().end() != message.members().find(client->current_node_id())) {
    t.execute(t1.insert(
    t1.object_id = message.object_id(),
      t1.object_size = message.object_size(),
      t1.state = orm::sync_state_dbo::state_t::follower,
      t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT()));

    for (const auto & member : message.members()) {
      t.execute(t2.insert(
        t2.object_id = message.object_id(),
        t2.member_node = member.first,
        t2.voted_for = message.leader_node(),
        t2.generation = message.generation(),
        t2.current_term = message.current_term(),
        t2.commit_index = message.commit_index(),
        t2.last_applied = message.last_applied(),
        t2.last_activity = std::chrono::system_clock::now()));
    }
  }
  else {
    return;
  }
  orm::sync_replica_map_dbo t3;
  t.execute(t3.delete_if(t3.object_id == message.object_id()));

  for (const auto & node : message.replica_map()) {
    for (const auto & replica : node.second) {
      t.execute(t3.insert(
        t3.object_id = message.object_id(),
        t3.replica = replica,
        t3.node = node.first,
        t3.last_access = std::chrono::system_clock::now()));
    }
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_add_message_request& message,
  const imessage_map::message_info_t & message_info) {

  auto client = sp.get<dht::network::client>();

  auto leader = this->get_leader(sp, t, message.object_id());
  if(leader && leader != message.leader_node()) {
    (*client)->send(
      sp,
      leader,
      messages::sync_add_message_request(
        message.object_id(),
        leader,
        message.source_node(),
        message.local_index(),
        message.message_type(),
        message.member_node(),
        message.replica()));
    return;
  }

  if (message.leader_node() != client->current_node_id()) {
    (*client)->send(
      sp,
      message.leader_node(),
      message);
    return;
  }

  this->add_to_log(
    sp,
    t,
    message.object_id(),
    message.message_type(),
    message.member_node(),
    message.replica(),
    message.source_node(),
    message.local_index());
}

void vds::dht::network::sync_process::apply_message(const service_provider& sp, database_transaction& t,
  const messages::sync_leader_broadcast_request& message,
  const imessage_map::message_info_t & message_info) {
  
  auto client = sp.get<dht::network::client>();
  if(base_message_type::successful == this->apply_base_message(sp, t, message, message_info)) {

  }
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::sync_leader_broadcast_response& message,
    const imessage_map::message_info_t & message_info) {
  throw vds_exceptions::invalid_operation();
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::sync_replica_operations_request& message,
    const imessage_map::message_info_t & message_info) {
  auto client = sp.get<dht::network::client>();

  if(this->apply_base_message(sp, t, message, message_info) == base_message_type::successful) {
    orm::sync_message_dbo t1;
    auto st = t.get_reader(t1.select(
      t1.message_type,
      t1.member_node,
      t1.replica,
      t1.source_node,
      t1.source_index)
      .where(t1.object_id == message.object_id()
        && t1.generation == message.generation()
        && t1.current_term == message.current_term()
        && t1.index == message.last_applied()));
    if(st.execute()) {
      vds_assert(
        t1.message_type.get(st) == message.message_type()
        && t1.member_node.get(st) == message.member_node()
        && t1.replica.get(st) == message.replica()
        && t1.source_node.get(st) == message.message_source_node()
        && t1.source_index.get(st) == message.message_source_index());
    }
    else {
      t.execute(t1.insert(
        t1.message_type = message.message_type(),
        t1.member_node = message.member_node(),
        t1.replica = message.replica(),
        t1.source_node = message.message_source_node(),
        t1.source_index = message.message_source_index(),
        t1.object_id = message.object_id(),
        t1.generation = message.generation(),
        t1.current_term = message.current_term(),
        t1.index = message.last_applied()));
    }
    (*client)->send(
      sp,
      message_info.source_node(),
      messages::sync_replica_operations_response(
        message.object_id(),
        message.generation(),
        message.current_term(),
        message.commit_index(),
        message.last_applied()));
  }
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp, database_transaction& t,
  const messages::sync_replica_operations_response& message,
    const imessage_map::message_info_t & message_info) {

  if(this->apply_base_message(sp, t, message, message_info)) {
    
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_offer_send_replica_operation_request & message,
  const imessage_map::message_info_t & message_info) {

  auto client = sp.get<dht::network::client>();

  if(this->apply_base_message(sp, t, message, message_info) != base_message_type::successful) {
    return;
  }

  send_replica(
      sp,
      t,
      message.target_node(),
      message.object_id(),
      message.replica(),
      message_info.source_node(),
      message.generation(),
      message.current_term(),
      message.commit_index(),
      message.last_applied());
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
  const messages::sync_offer_remove_replica_operation_request& message,
    const imessage_map::message_info_t & message_info) {

  auto client = sp.get<dht::network::client>();

  if (this->apply_base_message(sp, t, message, message_info) != base_message_type::successful) {
    return;
  }

  orm::chunk_replica_data_dbo t1;
  orm::device_record_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.replica_hash,
    t2.local_path)
    .inner_join(t2, t2.node_id == client->current_node_id() && t2.data_hash == t1.replica_hash)
    .where(t1.object_id == message.object_id() && t1.replica == message.replica()));
  if (!st.execute()) {
    sp.get<logger>()->trace(
      SyncModule,
      sp,
      "Remove replica %s:%d not found",
      base64::from_bytes(message.object_id()).c_str(),
      message.replica());
    return;
  }

  const auto replica_hash = t1.replica_hash.get(st);
  _client::delete_data(
    replica_hash,
    filename(t2.local_path.get(st)));

  t.execute(t1.delete_if(t1.object_id == message.object_id() && t1.replica == message.replica()));
  t.execute(t2.delete_if(t2.node_id == client->current_node_id() && t2.data_hash == replica_hash));

  this->add_local_log(
    sp,
    t,
    message.object_id(),
    orm::sync_message_dbo::message_type_t::remove_replica,
    client->current_node_id(),
    message.replica(),
    message_info.source_node());
}

void vds::dht::network::sync_process::send_random_replicas(
  std::map<uint16_t, std::list<std::function<void()>>> & allowed_replicas,
  std::set<uint16_t> & send_replicas) {
  while(!allowed_replicas.empty() && MIN_DISTRIBUTED_PIECES >= send_replicas.size()) {
    auto index = std::rand() % allowed_replicas.size();
    for (const auto & replica : allowed_replicas) {
      if (0 == index--) {
        vds_assert(send_replicas.end() == send_replicas.find(replica.first));
        vds_assert(!replica.second.empty());

        auto sender_index = std::rand() % replica.second.size();
        for(const auto & sender : replica.second) {
          if(0 == sender_index--) {
            sender();
            break;
          }
        }

        send_replicas.emplace(replica.first);
        allowed_replicas.erase(replica.first);
        break;
      }
    }
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_replica_request& message,
  const imessage_map::message_info_t & message_info) {

  auto client = sp.get<dht::network::client>();
  sp.get<logger>()->trace(
      SyncModule,
      sp,
      "%s: replica %s request from %s",
      base64::from_bytes(client->current_node_id()).c_str(),
      base64::from_bytes(message.object_id()).c_str(),
      base64::from_bytes(message_info.source_node()).c_str());

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(
    t1.select(
      t1.state,
      t2.voted_for,
      t2.generation,
      t2.current_term,
      t2.commit_index,
      t2.last_applied)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));
  if(!st.execute()) {
    sp.get<logger>()->trace(
        SyncModule,
        sp,
        "%s: replica %s request not found from %s",
        base64::from_bytes(client->current_node_id()).c_str(),
        base64::from_bytes(message.object_id()).c_str(),
        base64::from_bytes(message_info.source_node()).c_str());
    (*client)->send_closer(
      sp,
      message.object_id(),
      GENERATE_DISTRIBUTED_PIECES,
      message);
  }
  else {
    if (t1.state.get(st) != orm::sync_state_dbo::state_t::leader) {
      sp.get<logger>()->trace(
          SyncModule,
          sp,
          "%s: replica %s request from %s redirected to %s",
          base64::from_bytes(client->current_node_id()).c_str(),
          base64::from_bytes(message.object_id()).c_str(),
          base64::from_bytes(message_info.source_node()).c_str(),
          base64::from_bytes(t2.voted_for.get(st)).c_str());
      (*client)->send(
        sp,
        t2.voted_for.get(st),
        message);
    }
    else {
      const auto generation = t2.generation.get(st);
      const auto current_term = t2.current_term.get(st);
      const auto commit_index = t2.commit_index.get(st);
      const auto last_applied = t2.last_applied.get(st);

      //collect possible sources
      std::map<uint16_t, std::list<std::function<void(void)>>> allowed_replicas;
      
      orm::chunk_replica_data_dbo t5;
      orm::device_record_dbo t6;
      st = t.get_reader(
        t5
        .select(t5.replica, t5.replica_hash, t6.local_path)
        .inner_join(t6, t6.node_id == client->current_node_id() && t6.data_hash == t5.replica_hash)
        .where(t5.object_id == message.object_id()));

      while (st.execute()) {
        const auto replica = t5.replica.get(st);
        if (message.exist_replicas().end() == message.exist_replicas().find(replica)) {
          allowed_replicas[replica].push_back([
              client,
              generation,
              current_term,
              commit_index,
              last_applied,
              replica,
              replica_hash = t5.replica_hash.get(st),
              local_path = t6.local_path.get(st),
              sp,
              target_node = message_info.source_node(),
              object_id = message.object_id()]() {
              auto data = _client::read_data(
                replica_hash,
                filename(local_path));
              sp.get<logger>()->trace(
                SyncModule,
                sp,
                "Send replica %s:%d to %s",
                base64::from_bytes(object_id).c_str(),
                replica,
                base64::from_bytes(target_node).c_str());
              (*client)->send(
                sp,
                target_node,
                messages::sync_replica_data(
                  object_id,
                  client->current_node_id(),
                  generation,
                  current_term,
                  commit_index,
                  last_applied,
                  replica,
                  data));
            });
        }
      }

      orm::sync_replica_map_dbo t7;
      st = t.get_reader(t7.select(t7.replica, t7.node).where(t7.object_id == message.object_id()));
      while(st.execute()) {
        auto replica = t7.replica.get(st);
        if (message.exist_replicas().end() == message.exist_replicas().find(replica)
          && t7.node.get(st) != client->current_node_id()) {
          allowed_replicas[replica].push_back([
              client,
              replica,
              node = t7.node.get(st),
              sp,
              generation,
              current_term,
              commit_index,
              last_applied,
              target_node = message_info.source_node(),
              object_id = message.object_id()]() {
                sp.get<logger>()->trace(
                  SyncModule,
                  sp,
                  "Offer %s to send replica %s:%d to %s",
                  base64::from_bytes(node).c_str(),
                  base64::from_bytes(object_id).c_str(),
                  replica,
                  base64::from_bytes(target_node).c_str());
                (*client)->send(
                sp,
                node,
                messages::sync_offer_send_replica_operation_request(
                  object_id,
                  generation,
                  current_term,
                  commit_index,
                  last_applied,
                  replica,
                  target_node));
            });
        }
      }

      //Send replicas
      auto send_replicas = message.exist_replicas();
      this->send_random_replicas(allowed_replicas, send_replicas);

      if(MIN_DISTRIBUTED_PIECES >= send_replicas.size()) {
        allowed_replicas.clear();

        orm::chunk_dbo t3;
        orm::device_record_dbo t4;
        st = t.get_reader(
          t3
          .select(t4.local_path)
          .inner_join(t4, t4.node_id == client->current_node_id() && t4.data_hash == t3.replica_hash)
          .where(t3.object_id == message.object_id()));

        if (st.execute()) {
          auto data = file::read_all(filename(t4.local_path.get(st)));
          for (uint16_t replica = 0; replica < GENERATE_DISTRIBUTED_PIECES; ++replica) {
            if (send_replicas.end() == send_replicas.find(replica)) {
              allowed_replicas[replica].push_back([
                  this,
                  &t,
                  &t5,
                  client,
                  data,
                  generation,
                  current_term,
                  commit_index,
                  last_applied,
                  replica,
                  sp,
                  target_node = message_info.source_node(),
                  object_id = message.object_id()]() {
                  binary_serializer s;
                  this->distributed_generators_.find(replica)->second->write(s, data.data(), data.size());
                  const_data_buffer replica_data(s.data());
                  sp.get<logger>()->trace(
                    SyncModule,
                    sp,
                    "Send replica %s:%d to %s",
                    base64::from_bytes(object_id).c_str(),
                    replica,
                    base64::from_bytes(target_node).c_str());
                  (*client)->send(
                    sp,
                    target_node,
                    messages::sync_replica_data(
                      object_id,
                      client->current_node_id(),
                      generation,
                      current_term,
                      commit_index,
                      last_applied,
                      replica,
                      replica_data));

                  const auto data_hash = hash::signature(hash::sha256(), replica_data);
                  auto fn = _client::save_data(sp, t, data_hash, replica_data);

                  t.execute(
                    t5.insert(
                      t5.object_id = object_id,
                      t5.replica = replica,
                      t5.replica_hash = data_hash));
                });
            }
          }
        }
        this->send_random_replicas(allowed_replicas, send_replicas);
      }
    }
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_replica_data& message,
  const imessage_map::message_info_t & message_info) {
  auto client = sp.get<dht::network::client>();

  orm::chunk_replica_data_dbo t2;
  auto st = t.get_reader(
    t2.select(t2.replica_hash)
    .where(t2.object_id == message.object_id()
      && t2.replica == message.replica()));
  if (st.execute()) {
    //Already exists
    sp.get<logger>()->trace(
      SyncModule,
      sp,
      "Replica %s:%d from %s is already exists",
      base64::from_bytes(message.object_id()).c_str(),
      message.replica(),
      base64::from_bytes(message_info.source_node()).c_str());
  }
  else {
    const auto data_hash = hash::signature(hash::sha256(), message.data());
    auto fn = _client::save_data(sp, t, data_hash, message.data());
    sp.get<logger>()->trace(
      SyncModule,
      sp,
      "Got replica %s:%d from %s",
      base64::from_bytes(message.object_id()).c_str(),
      message.replica(),
      base64::from_bytes(message_info.source_node()).c_str());

    t.execute(
      t2.insert(
        t2.object_id = message.object_id(),
        t2.replica = message.replica(),
        t2.replica_hash = data_hash));

    switch (this->apply_base_message(sp, t, message, message_info)) {
    case base_message_type::not_found: {
      this->send_snapshot_request(sp, message.object_id(), message.leader_node());
      break;
    }
    case base_message_type::successful: {
      const auto leader = this->get_leader(sp, t, message.object_id());
      this->add_local_log(
        sp,
        t,
        message.object_id(),
        orm::sync_message_dbo::message_type_t::add_replica,
        client->current_node_id(),
        message.replica(),
        leader);
    }
    }
  }
}

void vds::dht::network::sync_process::send_leader_broadcast(
  const vds::service_provider& sp,
  vds::database_transaction& t,
  const const_data_buffer & object_id) {

  auto & client = *sp.get<dht::network::client>();
  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(
    t2.select(t2.member_node, t2.last_activity)
      .where(t2.object_id == object_id));
  std::set<const_data_buffer> to_remove;
  std::set<const_data_buffer> member_nodes;
  while (st.execute()) {
    const auto member_node = t2.member_node.get(st);
    if (member_node != client->current_node_id()) {
      const auto last_activity = t2.last_activity.get(st);

      if (std::chrono::system_clock::now() - last_activity > MEMBER_TIMEOUT()) {
        to_remove.emplace(member_node);
      }
      else {
        member_nodes.emplace(member_node);
      }
    }
  }

  if(to_remove.empty()) {
    st = t.get_reader(
      t2.select(t2.generation, t2.current_term, t2.commit_index, t2.last_applied)
      .where(t2.object_id == object_id && t2.member_node == client->current_node_id()));
    if(!st.execute()) {
      throw vds_exceptions::invalid_operation();
    }

    const auto generation = t2.generation.get(st);
    const auto current_term = t2.current_term.get(st);
    const auto commit_index = t2.commit_index.get(st);
    const auto last_applied = t2.last_applied.get(st);

    for (const auto & member_node : member_nodes) {
      sp.get<logger>()->trace(SyncModule, sp, "Send leader broadcast to %s", base64::from_bytes(member_node).c_str());
      client->send(
        sp,
        member_node,
        messages::sync_leader_broadcast_request(
          object_id,
          generation,
          current_term,
          commit_index,
          last_applied));
    }
  }
  else {
    //Remove members
    for (const auto & member_node : to_remove) {
      this->add_local_log(
        sp,
        t,
        object_id,
        orm::sync_message_dbo::message_type_t::remove_member,
        member_node,
        0,
        client->current_node_id());
    }
  }

  if(GENERATE_DISTRIBUTED_PIECES > member_nodes.size()) {
    st = t.get_reader(
      t1.select(t1.object_size, t2.generation, t2.current_term, t2.commit_index, t2.last_applied)
      .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
      .where(t1.object_id == object_id));
    if (!st.execute()) {
      throw vds_exceptions::invalid_operation();
    }

    const auto generation = t2.generation.get(st);
    const auto current_term = t2.current_term.get(st);
    const auto commit_index = t2.commit_index.get(st);
    const auto last_applied = t2.last_applied.get(st);
    const auto object_size = t1.object_size.get(st);

    client->send_near(
      sp,
      object_id,
      GENERATE_DISTRIBUTED_PIECES,
      messages::sync_looking_storage_request(
        object_id,
        generation,
        current_term,
        commit_index,
        last_applied,
        object_size),
      [&member_nodes](const dht_route<std::shared_ptr<dht_session>>::node & node)->bool {
        return member_nodes.end() == member_nodes.find(node.node_id_);
      });
  }

  t.execute(
    t1.update(
        t1.next_sync = std::chrono::system_clock::now() + FOLLOWER_TIMEOUT())
      .where(t1.object_id == object_id));
}

void vds::dht::network::sync_process::sync_entries(
  const service_provider& sp,
  database_transaction& t) {

  auto & client = *sp.get<dht::network::client>();

  std::map<const_data_buffer, orm::sync_state_dbo::state_t> objects;
  orm::sync_state_dbo t1;
  auto st = t.get_reader(
    t1.select(
      t1.object_id,
      t1.state)
    .where(t1.next_sync <= std::chrono::system_clock::now()));
  while (st.execute()) {
    const auto object_id = t1.object_id.get(st);
    objects[object_id] = t1.state.get(st);
  }

  for (auto & p : objects) {
    switch (p.second) {

    case orm::sync_state_dbo::state_t::follower: {
      this->make_new_election(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::canditate: {
      this->make_leader(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::leader: {
      this->send_leader_broadcast(sp, t, p.first);
      break;
    }

    }
  }
}

void vds::dht::network::sync_process::send_snapshot_request(
  const service_provider& sp,
  const const_data_buffer& object_id,
  const const_data_buffer& leader_node,
  const const_data_buffer& from_node) {

  auto & client = *sp.get<dht::network::client>();
  client->send(
    sp,
    leader_node,
    messages::sync_snapshot_request(
      object_id,
      ((!from_node) ? client.current_node_id() : from_node)));

}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_new_election_request & message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t2.voted_for, t2.generation, t2.current_term)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));

  if (st.execute()) {
    if (t2.generation.get(st) < message.generation()
      || (t2.generation.get(st) == message.generation() && t2.current_term.get(st) < message.current_term())) {
      this->make_follower(sp, t, message.object_id(), message.generation(), message.current_term(), message.source_node());
      return;
    }
    if (t2.generation.get(st) > message.generation()
      || (t2.generation.get(st) == message.generation() && t2.current_term.get(st) > message.current_term())) {
      if (t1.state.get(st) == orm::sync_state_dbo::state_t::leader) {
        send_snapshot(sp, t, message.object_id(), { message.source_node() });
      }
      else {
        this->send_snapshot_request(sp, message.object_id(), t2.voted_for.get(st), message.source_node());
      }
      return;
    }

    vds_assert(t2.generation.get(st) == message.generation() && t2.current_term.get(st) == message.current_term());
  }
  else {
    this->send_snapshot_request(sp, message.object_id(), message.source_node(), client->current_node_id());
  }
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_new_election_response & message,
  const imessage_map::message_info_t & message_info) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t2.voted_for, t2.generation, t2.current_term)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == message.object_id()));

  if (st.execute()
    && t1.state.get(st) == orm::sync_state_dbo::state_t::canditate
    && t2.generation.get(st) == message.generation()
    && t2.current_term.get(st) == message.current_term()) {

    st = t.get_reader(t2.select(t2.last_activity).where(
      t2.object_id == message.object_id()
      && t2.member_node == message.source_node()));
    if(!st.execute()) {
      t.execute(t2.insert(
        t2.object_id = message.object_id(),
        t2.member_node = message.source_node(),
        t2.voted_for = client->current_node_id(),
        t2.generation = message.generation(),
        t2.current_term = message.current_term(),
        t2.commit_index = 0,
        t2.last_applied = 0,
        t2.last_activity = std::chrono::system_clock::now()));
    }
    else {
      t.execute(t2.update(
        t2.voted_for = client->current_node_id(),
        t2.generation = message.generation(),
        t2.current_term = message.current_term(),
        t2.commit_index = 0,
        t2.last_applied = 0,
        t2.last_activity = std::chrono::system_clock::now())
        .where(t2.object_id == message.object_id() && t2.member_node == message.source_node()));
    }

    db_value<uint64_t> voted_count;
    st = t.get_reader(
      t2.select(
        db_count(t2.member_node).as(voted_count))
      .where(t2.object_id == message.object_id()
        && t2.voted_for == client->current_node_id()
        && t2.generation == message.generation()
        && t2.current_term == message.current_term()));
    if(!st.execute()) {
      throw vds_exceptions::invalid_operation();
    }

    const auto count = voted_count.get(st);
    if(count >= this->get_quorum(sp, t, message.object_id())) {
      this->make_leader(sp, t, message.object_id());
    }
  }
}


//void vds::dht::network::sync_process::apply_message(
//  const service_provider& sp,
//  database_transaction & t,
//  const messages::sync_coronation_request& message) {
//
//  orm::sync_state_dbo t1;
//  orm::sync_member_dbo t2;
//  auto st = t.get_reader(t1.select(
//    t1.state, t1.generation, t1.current_term)
//    .where(t1.object_id == base64::from_bytes(message.object_id())));
//
//  if(st.execute()) {
//    if (
//      message.generation() < t1.generation.get(st)
//      || (message.generation() == t1.generation.get(st) && message.current_term() < t1.current_term.get(st))) {
//      this->send_coronation_request(sp, t, message.object_id(), message.source_node());
//    }
//    else {
//      auto & client = *sp.get<dht::network::client>();
//      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
//        client->send(
//          sp,
//          message.source_node(),
//          messages::sync_member_operation_request(
//            message.object_id(),
//            client.current_node_id(),
//            t1.generation.get(st),
//            t1.current_term.get(st),
//            messages::sync_member_operation_request::operation_type_t::add_member));
//
//        t.execute(t2.delete_if(t2.object_id == message.object_id()));
//        t.execute(t1.delete_if(t1.object_id == message.object_id()));
//      } else {
//        this->make_follower(sp, t, message);
//      }
//    }
//  }
//  else {
//    this->make_follower(sp, t, message);
//  }
//
//  this->sync_object_->schedule(sp, [this, sp, message]() {
//    auto p = this->sync_entries_.find(message.object_id());
//    if (this->sync_entries_.end() == p) {
//      auto & entry = this->sync_entries_.at(message.object_id());
//      entry.make_follower(sp, message.object_id(), message.source_node(), message.current_term());
//
//      auto & client = *sp.get<dht::network::client>();
//      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
//        client->send(
//          sp,
//          message.source_node(),
//          messages::sync_coronation_response(
//            message.object_id(),
//            message.current_term(),
//            client.current_node_id()));
//      }
//    }
//    else if (p->second.current_term_ <= message.current_term()) {
//      p->second.make_follower(sp, message.object_id(), message.source_node(), message.current_term());
//
//      auto & client = *sp.get<dht::network::client>();
//      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
//        client->send(
//          sp,
//          message.source_node(),
//          messages::sync_coronation_response(
//            message.object_id(),
//            message.current_term(),
//            client.current_node_id()));
//      }
//    }
//    else {
//      auto & client = *sp.get<dht::network::client>();
//      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
//        client->send(
//          sp,
//          message.source_node(),
//          messages::sync_coronation_request(
//            message.object_id(),
//            message.current_term(),
//            std::set<const_data_buffer>(),
//            p->second.voted_for_));
//      }
//    }
//  });
//}
//
//void vds::dht::network::sync_process::apply_message(
//  const service_provider& sp,
//  const messages::sync_coronation_response& message) {
//
//}

vds::const_data_buffer
vds::dht::network::sync_process::get_leader(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const const_data_buffer &object_id) {
  auto client = sp.get<dht::network::client>();
  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(
      t1.select(
              t1.state,
              t2.voted_for)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
          .where(t1.object_id == object_id));
  if(st.execute()) {
    if(orm::sync_state_dbo::state_t::leader == static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st))){
      return client->current_node_id();
    }
    else {
      return t2.voted_for.get(st);
    }
  }

  return const_data_buffer();
}

void vds::dht::network::sync_process::apply_record(const vds::service_provider& sp, vds::database_transaction& t,
  const const_data_buffer& object_id, uint64_t generation, uint64_t current_term, uint64_t message_index) {
  orm::sync_message_dbo t1;
  auto st = t.get_reader(
    t1.select(
      t1.message_type,
      t1.replica,
      t1.member_node,
      t1.source_node,
      t1.source_index)
    .where(t1.object_id == object_id
      && t1.generation == generation
      && t1.current_term == current_term
      && t1.index == message_index));
  if(!st.execute()) {
    throw vds_exceptions::not_found();
  }

  this->apply_record(
    sp,
    t,
    object_id,
    t1.message_type.get(st),
    t1.member_node.get(st),
    t1.replica.get(st),
    t1.source_node.get(st),
    t1.source_index.get(st));

  const auto client = sp.get<dht::network::client>();
  orm::sync_member_dbo t2;
  t.execute(t2.update(
    t2.commit_index = message_index)
    .where(
      t2.object_id == object_id
      && t2.member_node == client->current_node_id()
      && t2.generation == generation
      && t2.current_term == current_term
      && t2.commit_index == message_index - 1));
  vds_assert(1 == t.rows_modified());
}

void vds::dht::network::sync_process::apply_record(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const const_data_buffer &object_id,
    orm::sync_message_dbo::message_type_t message_type,
    const const_data_buffer & member_node,
    uint16_t replica,
    const const_data_buffer & message_node,
    uint64_t message_index) {

  switch (message_type) {
    case orm::sync_message_dbo::message_type_t::add_member: {
      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Appply: Add member %s to store object %s",
        base64::from_bytes(member_node).c_str(),
        base64::from_bytes(object_id).c_str());

      orm::sync_member_dbo t1;
      auto st = t.get_reader(
          t1.select(t1.object_id)
              .where(
                  t1.object_id == object_id
                  && t1.member_node == member_node));

      if (!st.execute()) {
        t.execute(
            t1.insert(
                t1.object_id = object_id,
                t1.member_node = member_node));
      }

      this->send_snapshot(sp, t, object_id, { member_node });
      break;
    }

    case orm::sync_message_dbo::message_type_t::remove_member: {
      orm::sync_member_dbo t1;
      t.execute(
        t1.delete_if(
          t1.object_id == object_id
          && t1.member_node == member_node));

      break;
    }

    case orm::sync_message_dbo::message_type_t::add_replica:{
      sp.get<logger>()->trace(
        SyncModule,
        sp,
        "Appply: Add replica %s:%d to node %s",
        base64::from_bytes(object_id).c_str(),
        replica,
        base64::from_bytes(member_node).c_str());

      orm::sync_replica_map_dbo t1;
      auto st = t.get_reader(
          t1.select(t1.last_access)
              .where(
                  t1.object_id == object_id
                  && t1.node == member_node
                  && t1.replica == replica));
      if (!st.execute()) {
        t.execute(
            t1.insert(
                t1.object_id = object_id,
                t1.node = member_node,
                t1.replica = replica,
                t1.last_access = std::chrono::system_clock::now()));
      }

      break;
    }

    case orm::sync_message_dbo::message_type_t::remove_replica: {
      orm::sync_replica_map_dbo t1;
      t.execute(
        t1.delete_if(
          t1.object_id == object_id
          && t1.node == member_node
          && t1.replica == replica));

      break;
    }

    default:{
      throw std::runtime_error("Invalid operation");
    }
  }

  //remove local record
  const auto client = sp.get<dht::network::client>();
  if(client->current_node_id() == message_node) {
    orm::sync_local_queue_dbo t2;
    t.execute(t2.delete_if(t2.object_id == object_id && t2.local_index == message_index));
    vds_assert(t.rows_modified() == 1);
  }
}

void vds::dht::network::sync_process::send_snapshot(
  const service_provider& sp,
  database_read_transaction& t,
  const const_data_buffer& object_id,
  const std::set<vds::const_data_buffer> & target_nodes) {

  const auto client = sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(
    t1.select(
      t1.object_size,
      t1.state,
      t2.generation,
      t2.current_term,
      t2.commit_index,
      t2.last_applied)
    .inner_join(t2, t2.object_id == t1.object_id && t2.member_node == client->current_node_id())
    .where(t1.object_id == object_id));

  if(!st.execute() || orm::sync_state_dbo::state_t::leader != t1.state.get(st)) {
    for (const auto & target_node : target_nodes) {
      (*client)->send_closer(
        sp,
        object_id,
        GENERATE_DISTRIBUTED_PIECES,
        messages::sync_snapshot_request(object_id, target_node));
    }

    return;
  }

  const auto object_size = t1.object_size.get(st);
  const auto generation = t2.generation.get(st);
  const auto current_term = t2.current_term.get(st);
  const auto commit_index = t2.commit_index.get(st);
  const auto last_applied = t2.last_applied.get(st);

  //
  orm::sync_replica_map_dbo t3;
  st = t.get_reader(
    t3.select(
      t3.replica,
      t3.node)
    .where(t3.object_id == object_id));
  std::map<const_data_buffer, std::set<uint16_t>> replica_map;
  while(st.execute()) {
    replica_map[t3.node.get(st)].emplace(t3.replica.get(st));
  }

  //
  st = t.get_reader(
    t2.select(
      t2.member_node,
      t2.voted_for)
    .where(t2.object_id == object_id));
  std::map<const_data_buffer, messages::sync_snapshot_response::member_state> members;
  while (st.execute()) {
    auto & p = members[t2.member_node.get(st)];
    p.voted_for = t2.voted_for.get(st);
  }

  for (const auto & target_node : target_nodes) {
    (*client)->send(
      sp,
      target_node,
      messages::sync_snapshot_response(
        object_id,
        object_size,
        target_node,
        client->current_node_id(),
        generation,
        current_term,
        commit_index,
        last_applied,
        replica_map,
        members));
  }
}

void vds::dht::network::sync_process::sync_local_queues(
  const service_provider& sp,
  database_transaction& t) {
  const auto client = sp.get<dht::network::client>();

  orm::sync_local_queue_dbo t1;
  orm::sync_state_dbo t2;
  orm::sync_member_dbo t3;
  auto st = t.get_reader(t1.select(
    t1.local_index,
    t1.object_id,
    t1.message_type,
    t1.member_node,
    t1.replica,
    t3.voted_for)
    .inner_join(t2, t2.state == orm::sync_state_dbo::state_t::follower && t2.object_id == t1.object_id)
    .inner_join(t3, t3.object_id == t1.object_id && t3.member_node == client->current_node_id())
    .where(t1.last_send <= std::chrono::system_clock::now() - LOCAL_QUEUE_TIMEOUT()));
  std::set<uint64_t> processed;
  while (st.execute()) {
    (*client)->send(
      sp,
      t3.voted_for.get(st),
      messages::sync_add_message_request(
        t1.object_id.get(st),
        t3.voted_for.get(st),
        client->current_node_id(),
        t1.local_index.get(st),
        t1.message_type.get(st),
        t1.member_node.get(st),
        t1.replica.get(st)));
    processed.emplace(t1.local_index.get(st));
  }

  for(const auto local_index : processed) {
    t.execute(t1.update(t1.last_send = std::chrono::system_clock::now()).where(t1.local_index == local_index));
  }
}

void vds::dht::network::sync_process::sync_replicas(const service_provider& sp, database_transaction& t) {
  replica_sync sync;
  sync.load(sp, t);
  sync.normalize_density(sp, t);
}

void vds::dht::network::sync_process::replica_sync::load(const service_provider& sp, database_transaction& t) {
  const auto client = sp.get<dht::network::client>();

  orm::chunk_dbo t1;
  auto st = t.get_reader(
    t1
    .select(t1.object_id)
    .where(t1.last_sync <= std::chrono::system_clock::now() - std::chrono::minutes(10))
    .order_by(t1.last_sync));
  while (st.execute()) {
    this->register_local_chunk(t1.object_id.get(st), client->current_node_id());
  }

  orm::sync_replica_map_dbo t2;
  st = t.get_reader(t2.select(t2.object_id, t2.replica, t2.node));
  while (st.execute()) {
    this->register_replica(t2.object_id.get(st), t2.replica.get(st), t2.node.get(st));
  }

  orm::sync_state_dbo t3;
  orm::sync_member_dbo t4;
  st = t.get_reader(t3.select(
    t3.object_id,
    t3.state,
    t4.voted_for,
    t4.generation,
    t4.current_term,
    t4.commit_index,
    t4.last_applied)
    .inner_join(t4, t4.object_id == t3.object_id && t4.member_node == client->current_node_id()));
  while(st.execute()) {
    if(t3.state.get(st) == orm::sync_state_dbo::state_t::leader) {
      this->register_sync_leader(
        t3.object_id.get(st),
        client->current_node_id(),
        t4.generation.get(st),
        t4.current_term.get(st),
        t4.commit_index.get(st),
        t4.last_applied.get(st));
    }
    else {
      this->register_sync_leader(
        t3.object_id.get(st),
        t4.voted_for.get(st),
        t4.generation.get(st),
        t4.current_term.get(st),
        t4.commit_index.get(st),
        t4.last_applied.get(st));
    }
  }

  st = t.get_reader(t4.select(t4.object_id, t4.member_node));
  while (st.execute()) {
    this->register_sync_member(t4.object_id.get(st), t4.member_node.get(st));
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::restore_chunk(
  const vds::service_provider& sp,
  const std::map<uint16_t, std::set<vds::const_data_buffer>>& replica_nodes,
  const vds::const_data_buffer& object_id) const {
  const auto client = sp.get<dht::network::client>();

  //Ask for missing replicas
  std::set<uint16_t> exist_replicas;
  const auto current_node = this->nodes_.find(client->current_node_id());
  if(this->nodes_.end() != current_node) {
    exist_replicas = current_node->second.replicas_;
  }

  for (uint16_t replica = 0; replica < GENERATE_DISTRIBUTED_PIECES; ++replica) {
    if(exist_replicas.end() != exist_replicas.find(replica)) {
      continue;//exists already
    }

    const auto sources = replica_nodes.find(replica);
    if (replica_nodes.end() == sources){
      continue;//No sources
    }

    auto index = std::rand() % sources->second.size();
    for(const auto & source : sources->second) {
      if(index-- == 0) {
        (*client)->send(
          sp,
          source,
          messages::sync_replica_request(
            object_id,
            exist_replicas));
        break;
      }
    }
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::generate_missing_replicas(
  const vds::service_provider& sp,
  const database_read_transaction & t,
  const std::map<uint16_t, std::set<vds::const_data_buffer>>& replica_nodes,
  const vds::const_data_buffer& object_id,
  std::set<vds::const_data_buffer> chunk_nodes) const {
  const auto client = sp.get<dht::network::client>();

  //Detect members with minimal replicas
  std::map<std::size_t, std::set<const_data_buffer>> replica_count2nodes;
  for (const auto & node : this->nodes_) {
    replica_count2nodes[node.second.replicas_.size()].emplace(node.first);
  }

  //
  std::set<uint16_t> exist_replicas;
  for(const auto & replica : replica_nodes) {
    exist_replicas.emplace(replica.first);
  }

  uint16_t replica = 0;
  for(const auto & replica_count : replica_count2nodes) {
    for(const auto & node : replica_count.second) {
      //looking missing replica
      while(replica_nodes.end() != replica_nodes.find(replica)) {
        ++replica;
        if(replica >= GENERATE_DISTRIBUTED_PIECES) {
          break;
        }
      }
      if (replica >= GENERATE_DISTRIBUTED_PIECES) {
        break;
      }

      auto index = std::rand() % chunk_nodes.size();
      for(const auto & chunk_node : chunk_nodes) {
        if(chunk_node == client->current_node_id()){
          send_replica(
            sp,
            t,
            node,
            object_id,
            replica,
            this->sync_leader_,
            this->sync_generation_,
            this->sync_current_term_,
            this->sync_commit_index_,
            this->sync_last_applied_);
        }
        else {
          vds_assert(this->sync_leader_ == client->current_node_id());
          (*client)->send(
              sp,
              chunk_node,
              messages::sync_offer_send_replica_operation_request(
                  object_id,
                  this->sync_generation_,
                  this->sync_current_term_,
                  this->sync_commit_index_,
                  this->sync_last_applied_,
                  replica,
                  node));
        }
        if(index-- == 0) {
          break;
        }
      }
    }
    if (replica >= GENERATE_DISTRIBUTED_PIECES) {
      break;
    }
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::restore_replicas(
  const vds::service_provider& sp,
  const database_read_transaction & t,
  const std::map<uint16_t, std::set<vds::const_data_buffer>> & replica_nodes,
  const const_data_buffer & object_id) const {

  const auto client = sp.get<dht::network::client>();
  //How can generate replicas?
  std::set<const_data_buffer> chunk_nodes;
  for (const auto & node : this->nodes_) {
    if(node.second.replicas_.size() >= MIN_DISTRIBUTED_PIECES) {
      chunk_nodes.emplace(node.first);
      break;
    }
  }

  if(chunk_nodes.empty()) {
    this->restore_chunk(sp, replica_nodes, object_id);
  }
  else {
    this->generate_missing_replicas(sp, t, replica_nodes, object_id, chunk_nodes);
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::normalize_density(
  const vds::service_provider& sp,
  const std::map<uint16_t, std::set<vds::const_data_buffer>>& replica_nodes,
  const const_data_buffer& object_id) const {

  const auto client = sp.get<dht::network::client>();

  vds_assert(!this->nodes_.empty());
  auto target_count = GENERATE_DISTRIBUTED_PIECES / this->nodes_.size();
  if (0 == target_count) {
    target_count = 1;
  }
  sp.get<logger>()->trace(
      SyncModule,
      sp,
      "normalize object %s density to %d replicas",
      base64::from_bytes(object_id).c_str(),
      target_count);


  std::map<std::size_t, std::set<const_data_buffer>> replica_count;
  for (const auto & node : this->nodes_) {
    replica_count[node.second.replicas_.size()].emplace(node.first);
  }

  auto head = replica_count.cbegin();
  auto tail = replica_count.crbegin();

  while (
    head != replica_count.cend()
    && tail != replica_count.crend()
    && head->first < target_count
    && tail->first > target_count) {

    auto head_node = head->second.cbegin();
    if(head->second.cend() == head_node) {
      ++head;
      continue;
    }

    auto tail_node = tail->second.cbegin();
    if(tail->second.cend() == tail_node) {
      ++tail;
      continue;
    }

#ifdef max
#undef max
#endif

    for (;;) {
      std::size_t minimal_repilica_count = std::numeric_limits<std::size_t>::max();
      uint16_t minimal_replica;

      for(const auto replica : this->nodes_.at(*tail_node).replicas_) {
        if(minimal_repilica_count > replica_nodes.at(replica).size()) {
          minimal_repilica_count = replica_nodes.at(replica).size();
          minimal_replica = replica;
        }
      }
      vds_assert(minimal_repilica_count != std::numeric_limits<std::size_t>::max());

      sp.get<logger>()->trace(
          SyncModule,
          sp,
          "offer send replica %s:%d from %s to %s",
          base64::from_bytes(object_id).c_str(),
          minimal_replica,
          base64::from_bytes(*tail_node).c_str(),
          base64::from_bytes(*head_node).c_str());
      vds_assert(this->sync_leader_ == client->current_node_id());
      (*client)->send(
        sp,
        *tail_node,
        messages::sync_offer_send_replica_operation_request(
          object_id,
          this->sync_generation_,
          this->sync_current_term_,
          this->sync_commit_index_,
          this->sync_last_applied_,
          minimal_replica,
          *head_node));

      ++head_node;
      while (head->second.cend() == head_node) {
        ++head;
        if (
          head == replica_count.cend()
          || head->first >= target_count) {
          break;
        }

        head_node = head->second.cbegin();
      }
      if (
        head == replica_count.cend()
        || head->first >= target_count) {
        break;
      }

      ++tail_node;
      while (tail->second.cend() == tail_node) {
        ++tail;
        if (
          tail == replica_count.crend()
          || tail->first <= target_count) {
          break;
        }

        tail_node = tail->second.cbegin();
      }
      if (
        tail == replica_count.crend()
        || tail->first <= target_count) {
        break;
      }
    }
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::remove_duplicates(
  const vds::service_provider& sp,
  const std::map<uint16_t, std::set<vds::const_data_buffer>>& replica_nodes,
  const const_data_buffer& object_id) const {

  const auto client = sp.get<dht::network::client>();

  std::map<std::size_t, std::set<uint16_t>> replica_counts;
  for (const auto & node : replica_nodes) {
    replica_counts[node.second.size()].emplace(node.first);
  }

  for (const auto & replica_count : replica_counts) {
    if (1 < replica_count.first) {
      for (const auto replica : replica_count.second) {
        std::size_t max_replica_count = 0;
        std::set<const_data_buffer> most_replica_nodes;
        for (const auto & node : replica_nodes.find(replica)->second) {
          const auto count = this->nodes_.find(node)->second.replicas_.size();
          if (max_replica_count < count) {
            max_replica_count = count;
            most_replica_nodes.clear();
            most_replica_nodes.emplace(node);
          }
          else if (max_replica_count == count) {
            most_replica_nodes.emplace(node);
          }
        }

        vds_assert(0 < max_replica_count);
        if (max_replica_count > 1) {
          for (const auto & node : most_replica_nodes) {
            sp.get<logger>()->trace(
                SyncModule,
                sp,
                "offer remove replica %s:%d from %s",
                base64::from_bytes(object_id).c_str(),
                replica,
                base64::from_bytes(node).c_str());

            vds_assert(this->sync_leader_ == client->current_node_id());
            (*client)->send(
              sp,
              node,
              messages::sync_offer_remove_replica_operation_request(
                object_id,
                this->sync_generation_,
                this->sync_current_term_,
                this->sync_commit_index_,
                this->sync_last_applied_,
                node,
                replica));
          }
        }
      }
    }
  }
}

void vds::dht::network::sync_process::replica_sync::object_info_t::try_to_attach(
  const service_provider& sp,
  const const_data_buffer& object_id) const {

  const auto client = sp.get<dht::network::client>();
  const auto p = this->nodes_.find(client->current_node_id());
  if (this->nodes_.end() != p) {

    (*client)->send_near(
      sp,
      object_id,
      GENERATE_DISTRIBUTED_PIECES,
      messages::sync_looking_storage_response(
        object_id,
        p->second.replicas_));
  }
}

void vds::dht::network::sync_process::replica_sync::register_local_chunk(
  const const_data_buffer& object_id, const const_data_buffer& current_node_id) {
  auto & p = this->objects_[object_id].nodes_[current_node_id];
  for (uint16_t replica = 0; replica < GENERATE_DISTRIBUTED_PIECES; ++replica) {
    p.replicas_.emplace(replica);
  }
}

void vds::dht::network::sync_process::replica_sync::register_replica(const const_data_buffer& object_id,
  uint16_t replica, const const_data_buffer& node_id) {
  auto & p = this->objects_[object_id].nodes_[node_id];
  if(p.replicas_.end() == p.replicas_.find(replica)) {
    p.replicas_.emplace(replica);
  }
}

void vds::dht::network::sync_process::replica_sync::register_sync_leader(
  const const_data_buffer& object_id,
  const const_data_buffer& leader_node_id,
  uint64_t generation,
  uint64_t current_term,
  uint64_t commit_index,
  uint64_t last_applied) {
  auto & p = this->objects_[object_id];
  vds_assert(!p.sync_leader_);
  p.sync_leader_ = leader_node_id;
  p.sync_generation_ = generation;
  p.sync_current_term_ = current_term;
  p.sync_commit_index_ = commit_index;
  p.sync_last_applied_ = last_applied;
}

void vds::dht::network::sync_process::replica_sync::register_sync_member(
  const const_data_buffer& object_id,
  const const_data_buffer& member_node) {
  auto & p = this->objects_[object_id];
  if(p.nodes_.end() == p.nodes_.find(member_node)) {
    p.nodes_[member_node];//just create record
  }
}

void vds::dht::network::sync_process::replica_sync::normalize_density(
  const service_provider& sp,
  database_transaction& t) {
  const auto client = sp.get<dht::network::client>();
  for(const auto & object : this->objects_) {
    //Send chunks if this node is not in memebers
    if(!object.second.sync_leader_) {
      sp.get<logger>()->trace(
          SyncModule,
          sp,
          "This node has replicas %s without leader",
          base64::from_bytes(object.first).c_str());

      object.second.try_to_attach(sp, object.first);
    }
    else if(object.second.sync_leader_ == client->current_node_id()) {
      std::map<uint16_t, std::set<const_data_buffer>> replica_nodes;
      for(const auto & node : object.second.nodes_) {
        for(const auto replica : node.second.replicas_) {
          replica_nodes[replica].emplace(node.first);
        }
      }

      //Some replicas has been lost
      if(replica_nodes.size() < GENERATE_DISTRIBUTED_PIECES) {
        sp.get<logger>()->trace(
            SyncModule,
            sp,
            "object %s have %d replicas",
            base64::from_bytes(object.first).c_str(),
            replica_nodes.size());
        object.second.restore_replicas(sp, t, replica_nodes, object.first);
      }
      else {//All replicas exists
        object.second.normalize_density(sp, replica_nodes, object.first);
        object.second.remove_duplicates(sp, replica_nodes, object.first);
      }
    }
  }
}

void vds::dht::network::sync_process::make_leader(
  const service_provider& sp,
  database_transaction& t,
  const const_data_buffer& object_id) {
  const auto client = sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  t.execute(t1.update(
    t1.state = orm::sync_state_dbo::state_t::leader,
    t1.next_sync = std::chrono::system_clock::now() + FOLLOWER_TIMEOUT())
    .where(t1.object_id == object_id));

  this->send_snapshot(sp, t, object_id, this->get_members(sp, t, object_id));
}

void vds::dht::network::sync_process::make_follower(const service_provider& sp, database_transaction& t,
  const const_data_buffer& object_id, uint64_t generation, uint64_t current_term,
  const const_data_buffer& leader_node) {

  orm::sync_state_dbo t1;
  t.execute(t1.update(
    t1.state = orm::sync_state_dbo::state_t::follower,
    t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT())
    .where(t1.object_id == object_id));

  const auto client = sp.get<dht::network::client>();

  orm::sync_member_dbo t2;
  t.execute(t2.update(
    t2.generation = generation,
    t2.current_term = current_term,
    t2.commit_index = 0,
    t2.last_applied = 0,
    t2.last_activity = std::chrono::system_clock::now())
  .where(t2.object_id == object_id && t2.member_node == client->current_node_id()));
}

void vds::dht::network::sync_process::send_replica(
    const vds::service_provider &sp,
    const database_read_transaction & t,
    const const_data_buffer & target_node,
    const const_data_buffer & object_id,
    uint16_t replica,
    const const_data_buffer& leader_node_id,
    uint64_t generation,
    uint64_t current_term,
    uint64_t commit_index,
    uint64_t last_applied) {

  const auto client = sp.get<dht::network::client>();

  orm::chunk_replica_data_dbo t1;
  orm::device_record_dbo t2;
  auto st = t.get_reader(t1.select(
          t1.replica_hash,
          t2.local_path)
  .inner_join(t2, t2.node_id == client->current_node_id() && t2.data_hash == t1.replica_hash)
  .where(t1.object_id == object_id && t1.replica == replica));
  if(!st.execute()) {
    return;
  }

  const auto data = _client::read_data(
      t1.replica_hash.get(st),
      filename(t2.local_path.get(st)));

  (*client)->send(
      sp,
      target_node,
      messages::sync_replica_data(
          object_id,
          leader_node_id,
          generation,
          current_term,
          commit_index,
          last_applied,
          replica,
          data));
}
