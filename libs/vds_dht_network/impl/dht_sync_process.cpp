/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_sync_process.h"
#include "dht_network_client.h"
#include "messages/transaction_log_state.h"
#include "transaction_log_record_dbo.h"
#include "chunk_dbo.h"
#include "private/dht_network_client_p.h"
#include "transaction_log_unknown_record_dbo.h"
#include "messages/transaction_log_request.h"
#include "messages/transaction_log_record.h"
#include "messages/offer_replica.h"
#include "transaction_log.h"
#include "messages/got_replica.h"
#include "chunk_replica_data_dbo.h"
#include "messages/object_request.h"
#include "sync_replica_map_dbo.h"
#include "messages/sync_new_election.h"
#include "messages/sync_coronation.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "messages/sync_apply_operation.h"
#include "messages/sync_leader_broadcast.h"
#include "sync_message_dbo.h"
#include "messages/sync_replica_operations.h"
#include "messages/sync_looking_storage.h"
#include "db_model.h"
#include "device_config_dbo.h"
#include "messages/sync_snapshot.h"


void vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {

  this->sync_entries(sp, t);
}

std::set<vds::const_data_buffer> vds::dht::network::sync_process::get_members(
  const service_provider& sp,
  database_transaction& t,
  const vds::const_data_buffer& object_id) {

  orm::sync_member_dbo t1;
  auto st = t.get_reader(t1.select(t1.member_node).where(t1.object_id == object_id));

  std::set<vds::const_data_buffer> result;
  while(st.execute()) {
    result.emplace(t1.member_node.get(st));
  }

  return result;
}

bool vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_base_message_request & message) {

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t1.generation, t1.current_term, t1.voted_for, t1.last_applied, t1.commit_index)
    .where(t1.object_id == message.object_id()));

  if (st.execute()) {
    if (
      message.generation() > t1.generation.get(st)
      || (message.generation() == t1.generation.get(st) && message.current_term() > t1.current_term.get(st))) {
      send_snapshot_request(sp, message.object_id(), message.leader_node());
      return false;
    }
    else if (
      message.generation() < t1.generation.get(st)
      || (message.generation() == t1.generation.get(st) && message.current_term() < t1.current_term.get(st))) {

      auto & client = *sp.get<dht::network::client>();
      const auto leader = this->get_leader(sp, t, message.object_id());
      if(!leader || client->current_node_id() == leader) {
        send_snapshot(sp, t, message.object_id(), message.source_node());
      }
      else {
        send_snapshot_request(sp, message.object_id(), message.leader_node(), message.source_node());
      }

      return false;
    }
    else if(
      message.generation() == t1.generation.get(st)
      && t1.voted_for.get(st) != message.leader_node()) {
        this->make_new_election(sp, t);
      return false;
    }
    else if(message.generation() == t1.generation.get(st)
      && message.current_term() == t1.current_term.get(st)) {
      
      if(t1.state.get(st) == orm::sync_state_dbo::state_t::leader) {
      }

    }
  }
  else {
    return false;
  }

  return true;
}

void vds::dht::network::sync_process::add_sync_entry(
    const service_provider &sp,
    database_transaction &t,
    const const_data_buffer &object_id,
    uint32_t object_size) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  t.execute(t1.insert(
    t1.object_id = object_id,
    t1.object_size = object_size,
    t1.state = orm::sync_state_dbo::state_t::leader,
    t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
    t1.voted_for = client.current_node_id(),
    t1.generation = 0,
    t1.current_term = 0,
    t1.commit_index = 0,
    t1.last_applied = 0));

  sp.get<logger>()->trace(ThisModule, sp, "Make leader %s:0:0", base64::from_bytes(object_id).c_str());
  std::map<const_data_buffer, std::set<uint16_t>> members_map;
  orm::sync_replica_map_dbo t2;
  for(uint16_t i = 0; i < _client::GENERATE_DISTRIBUTED_PIECES; ++i) {
    t.execute(t2.insert(
      t2.object_id = object_id,
      t2.replica = i,
      t2.node = client.current_node_id(),
      t2.last_access = std::chrono::system_clock::now()));

    members_map[client.current_node_id()].emplace(i);
  }

  client->send_near(
    sp,
    object_id,
    _client::GENERATE_DISTRIBUTED_PIECES,
    messages::sync_looking_storage_request(
      object_id,
      client.current_node_id(),
      object_size));
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    const messages::sync_looking_storage_request & message) {

  auto & client = *sp.get<dht::network::client>();
  client->send_closer(
      sp,
      message.object_id(),
      _client::GENERATE_DISTRIBUTED_PIECES,
      message);

  sp.get<db_model>()->async_read_transaction(sp, [sp, message](database_read_transaction & t) {
    auto & client = *sp.get<dht::network::client>();

    for(const auto & record : orm::device_config_dbo::get_free_space(t, client.current_node_id())) {
      if(record.used_size + message.object_size() < record.reserved_size
         && message.object_size() < record.free_size) {

        std::set<uint16_t> replicas;
        orm::chunk_replica_data_dbo t1;
        auto st = t.get_reader(t1.select(t1.replica).where(t1.object_id == message.object_id()));
        while(st.execute()) {
          replicas.emplace(t1.replica.get(st));
        }

        client->send(
            sp,
            message.leader_node(),
            messages::sync_looking_storage_response(
                message.object_id(),
                message.leader_node(),
                client.current_node_id(),
                replicas));

        return;
      }
    }
  });
}

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction & t,
    const messages::sync_looking_storage_response & message) {

  auto & client = *sp.get<dht::network::client>();
  if (message.leader_node() != client.current_node_id()) {
    client->send(
        sp,
        message.leader_node(),
        message);
    return;
  }

  orm::sync_state_dbo t1;
  auto st = t.get_reader(
      t1.select(
              t1.state,
              t1.generation,
              t1.current_term,
              t1.commit_index,
              t1.last_applied,
              t1.voted_for)
          .where(t1.object_id == message.object_id()));

  if(!st.execute()) {
    return;
  }

  if (orm::sync_state_dbo::state_t::leader != static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st))) {
    const auto leader = t1.voted_for.get(st);
    client->send(
        sp,
        leader,
        messages::sync_looking_storage_response(
            message.object_id(),
            leader,
            message.source_node(),
            message.replicas()));

    return;
  }
  const auto generation = t1.generation.get(st);
  const auto current_term = t1.current_term.get(st);
  const auto commit_index = t1.commit_index.get(st);
  auto index = t1.last_applied.get(st);

  orm::sync_message_dbo t2;
  t.execute(
      t2.insert(
          t2.object_id = message.object_id(),
          t2.generation = generation,
          t2.current_term = current_term,
          t2.index = index,
          t2.message_type = orm::sync_message_dbo::message_type_t::add_member,
          t2.member_node = message.source_node()
  ));
  this->send_to_members(
      sp,
      t,
      messages::sync_replica_operations_request(
          message.object_id(),
          client->current_node_id(),
          generation,
          current_term,
          commit_index,
          index++,
          orm::sync_message_dbo::message_type_t::add_member,
          message.source_node(),
          0).serialize());

  if (!message.replicas().empty()) {
    //Register replica
    for (auto replica : message.replicas()) {
      t.execute(
          t2.insert(
              t2.object_id = message.object_id(),
              t2.generation = generation,
              t2.current_term = current_term,
              t2.index = index,
              t2.message_type = orm::sync_message_dbo::message_type_t::add_replica,
              t2.member_node = message.source_node(),
              t2.replica = replica
          ));
      this->send_to_members(
          sp,
          t,
          messages::sync_replica_operations_request(
              message.object_id(),
              client->current_node_id(),
              generation,
              current_term,
              commit_index,
              index++,
              orm::sync_message_dbo::message_type_t::add_replica,
              message.source_node(),
              replica).serialize());
    }
  }

  t.execute(
      t1.update(
          t1.last_applied = index)
      .where(t1.object_id == message.object_id()));

}

struct object_info_t {
  vds::orm::sync_state_dbo::state_t state;
  uint64_t generation;
  uint64_t current_term;
  uint64_t commit_index;
  uint64_t last_applied;
  uint64_t object_size;
};

void vds::dht::network::sync_process::sync_entries(
  const service_provider& sp,
  database_transaction& t) {

  std::map<const_data_buffer, object_info_t> objects;
  orm::sync_state_dbo t1;
  auto st = t.get_reader(
    t1.select(
      t1.object_id,
      t1.object_size,
      t1.state,
      t1.generation,
      t1.current_term,
      t1.commit_index,
      t1.last_applied)
    .where(t1.next_sync <= std::chrono::system_clock::now()));
  while (st.execute()) {
    const auto object_id = t1.object_id.get(st);
    auto & p = objects.at(object_id);
    p.state = static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st));
    p.generation = t1.generation.get(st);
    p.current_term = t1.current_term.get(st);
    p.commit_index = t1.commit_index.get(st);
    p.last_applied = t1.last_applied.get(st);
    p.object_size = t1.object_size.get(st);
  }

  for (auto & p : objects) {
    switch (p.second.state) {

    case orm::sync_state_dbo::state_t::follower: {
      this->make_candidate(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::canditate: {
      this->make_leader(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::leader: {
      auto & client = *sp.get<dht::network::client>();
      //Send leader broadcast
      orm::sync_member_dbo t2;
      st = t.get_reader(
        t2.select(t2.member_node, t2.last_activity)
        .where(t2.object_id == p.first));
      std::set<const_data_buffer> to_remove;
      std::set<const_data_buffer> member_nodes;
      while (st.execute()) {
        const auto member_node = t2.member_node.get(st);
        
        const auto last_activity = t2.last_activity.get(st);
        if(std::chrono::system_clock::now() - last_activity > MEMBER_TIMEOUT) {
          to_remove.emplace(member_node);
        }
        else {
          member_nodes.emplace(member_node);
        }
      }

      if(to_remove.empty()) {
        for (auto member_node : to_remove) {
          sp.get<logger>()->trace(ThisModule, sp, "Send leader broadcast to %s", base64::from_bytes(member_node).c_str());

          client->send(
            sp,
            member_node,
            messages::sync_leader_broadcast_request(
              p.first,
              client.current_node_id(),
              p.second.generation,
              p.second.current_term,
              p.second.commit_index,
              p.second.last_applied));
        }
      }
      else {
        //Remove members
        for (auto member_node : to_remove) {
          orm::sync_message_dbo t3;
          t.execute(
            t3.insert(
              t3.object_id = p.first,
              t3.generation = p.second.generation,
              t3.current_term = p.second.current_term,
              t3.index = p.second.last_applied++,
              t3.message_type = orm::sync_message_dbo::message_type_t::remove_member,
              t3.member_node = member_node));

          for (auto member : member_nodes) {
            client->send(
              sp,
              member,
              messages::sync_replica_operations_request(
                p.first,
                client.current_node_id(),
                p.second.generation,
                p.second.current_term,
                p.second.commit_index,
                p.second.last_applied,
                orm::sync_message_dbo::message_type_t::remove_member,
                member_node,
                0));
          }
        }
      }

      if(_client::GENERATE_DISTRIBUTED_PIECES > member_nodes.size()) {
        client->send_near(
          sp,
          p.first,
          _client::GENERATE_DISTRIBUTED_PIECES,
          messages::sync_looking_storage_request(
            p.first,
            client.current_node_id(),
            p.second.object_size),
          [&member_nodes](const dht_route<std::shared_ptr<dht_session>>::node & node)->bool {
            return member_nodes.end() == member_nodes.find(node.node_id_);
          });
      }

      t.execute(
        t1.update(
          t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
          t1.last_applied = p.second.last_applied)
        .where(t1.object_id == p.first));

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
  const messages::sync_new_election_request & message) {

  this->sync_object_->schedule(sp, [this, sp, message]() {
    auto p = this->sync_entries_.find(message.object_id());
    if (this->sync_entries_.end() != p) {
      if (p->second.current_term_ < message.current_term()) {
        p->second.make_follower(
          sp,
          message.object_id(),
          message.source_node(),
          message.current_term());

        auto & client = *sp.get<dht::network::client>();
        client->send(
          sp,
          message.source_node(),
          messages::sync_new_election_response(
            message.object_id(),
            message.current_term(),
            client.current_node_id()));

      }
    }
    else {
      this->sync_entries_[message.object_id()].make_follower(
        sp,
        message.object_id(),
        message.source_node(),
        message.current_term());

      auto & client = *sp.get<dht::network::client>();
      client->send(
        sp,
        message.source_node(),
        messages::sync_new_election_response(
          message.object_id(),
          message.current_term(),
          client.current_node_id()));
    }

    auto & client = *sp.get<dht::network::client>();
    client->send_closer(
      sp,
      message.object_id(),
      _client::GENERATE_DISTRIBUTED_PIECES,
      message);
  });
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  const messages::sync_new_election_response & message) {

  this->sync_object_->schedule(sp, [this, sp, message]() {
    auto p = this->sync_entries_.find(message.object_id());
    if (this->sync_entries_.end() != p) {
      if (p->second.current_term_ == message.current_term()
        && p->second.member_notes_.end() == p->second.member_notes_.find(message.source_node())) {

        p->second.member_notes_.emplace(message.source_node());

        if (p->second.quorum_ < p->second.member_notes_.size()) {
          p->second.make_leader(sp, message.object_id());
        }
      }
    }
  });
}

void vds::dht::network::sync_process::make_new_follower(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_coronation_request& message) {

  orm::sync_state_dbo t1;
  t.execute(t1.insert(
    t1.object_id = message.object_id(),
    t1.state = orm::sync_state_dbo::state_t::follower,
    t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
    t1.voted_for = message.source_node(),
    t1.generation = message.generation(),
    t1.current_term = message.current_term(),
    t1.commit_index = 0,
    t1.last_applied = 0));

  orm::sync_replica_map_dbo t2;
  for(auto p : message.member_notes()) {
    for (auto replica : p.second) {
      t.execute(t2.insert(
        t2.object_id = message.object_id(),
        t2.replica = replica,
        t2.node = p.first,
        t2.last_access = std::chrono::system_clock::now()));
    }
  }

  sp.get<logger>()->trace(
    SyncModule,
    sp,
    "Make follower %s:%d:%d",
    base64::from_bytes(message.object_id()).c_str(),
    message.generation(),
    message.current_term());

  auto & client = *sp.get<dht::network::client>();
  client->send(
    sp,
    message.source_node(),
    messages::sync_apply_operation_request(
      message.object_id(),
      client.current_node_id(),
      message.generation(),
      message.current_term(),
      0,
      0
      ));
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction & t,
  const messages::sync_coronation_request& message) {

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t1.generation, t1.current_term)
    .where(t1.object_id == base64::from_bytes(message.object_id())));

  if(st.execute()) {
    if (
      message.generation() < t1.generation.get(st)
      || (message.generation() == t1.generation.get(st) && message.current_term() < t1.current_term.get(st))) {
      this->send_coronation_request(sp, t, message.object_id(), message.source_node());
    }
    else {
      auto & client = *sp.get<dht::network::client>();
      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
        client->send(
          sp,
          message.source_node(),
          messages::sync_member_operation_request(
            message.object_id(),
            client.current_node_id(),
            t1.generation.get(st),
            t1.current_term.get(st),
            messages::sync_member_operation_request::operation_type_t::add_member));

        t.execute(t2.delete_if(t2.object_id == message.object_id()));
        t.execute(t1.delete_if(t1.object_id == message.object_id()));
      } else {
        this->make_follower(sp, t, message);
      }
    }
  }
  else {
    this->make_follower(sp, t, message);
  }

  this->sync_object_->schedule(sp, [this, sp, message]() {
    auto p = this->sync_entries_.find(message.object_id());
    if (this->sync_entries_.end() == p) {
      auto & entry = this->sync_entries_.at(message.object_id());
      entry.make_follower(sp, message.object_id(), message.source_node(), message.current_term());

      auto & client = *sp.get<dht::network::client>();
      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
        client->send(
          sp,
          message.source_node(),
          messages::sync_coronation_response(
            message.object_id(),
            message.current_term(),
            client.current_node_id()));
      }
    }
    else if (p->second.current_term_ <= message.current_term()) {
      p->second.make_follower(sp, message.object_id(), message.source_node(), message.current_term());

      auto & client = *sp.get<dht::network::client>();
      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
        client->send(
          sp,
          message.source_node(),
          messages::sync_coronation_response(
            message.object_id(),
            message.current_term(),
            client.current_node_id()));
      }
    }
    else {
      auto & client = *sp.get<dht::network::client>();
      if (message.member_notes().end() == message.member_notes().find(client->current_node_id())) {
        client->send(
          sp,
          message.source_node(),
          messages::sync_coronation_request(
            message.object_id(),
            message.current_term(),
            std::set<const_data_buffer>(),
            p->second.voted_for_));
      }
    }
  });
}

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  const messages::sync_coronation_response& message) {

}

vds::const_data_buffer
vds::dht::network::sync_process::get_leader(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const const_data_buffer &object_id) {

  orm::sync_state_dbo t1;
  auto st = t.get_reader(
      t1.select(
              t1.state,
              t1.voted_for)
          .where(t1.object_id == object_id));
  if(st.execute()) {
    if(orm::sync_state_dbo::state_t::leader == static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st))){
      auto client = sp.get<dht::network::client>();
      return client->current_node_id();
    }
    else {
      return t1.voted_for.get(st);
    }
  }

  return const_data_buffer();
}

void vds::dht::network::sync_process::apply_record(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const const_data_buffer &object_id,
    orm::sync_message_dbo::message_type_t message_type,
    const const_data_buffer & member_node,
    uint16_t replica) {
  switch (message_type) {
    case orm::sync_message_dbo::message_type_t::add_member: {
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
                t1.member_node = member_node,
                t1.last_activity = std::chrono::system_clock::now()));
      }

      break;
    }

    case orm::sync_message_dbo::message_type_t::add_replica:{
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

    default:{
      throw std::runtime_error("Invalid operation");
    }
  }
}

void vds::dht::network::sync_process::send_snapshot(
  const service_provider& sp,
  database_read_transaction& t,
  const const_data_buffer& object_id,
  const const_data_buffer& target_node) {

  orm::sync_state_dbo t1;
  auto st = t.get_reader(
    t1.select(
      t1.object_size,
      t1.state,
      t1.generation,
      t1.current_term,
      t1.commit_index,
      t1.last_applied)
    .where(t1.object_id == object_id));

  if(!st.execute() || orm::sync_state_dbo::state_t::leader != static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st))) {
    return;
  }

  const auto object_size = t1.object_size.get(st);
  const auto generation = t1.generation.get(st);
  const auto current_term = t1.current_term.get(st);
  const auto commit_index = t1.commit_index.get(st);
  const auto last_applied = t1.last_applied.get(st);

  //
  orm::sync_replica_map_dbo t2;
  st = t.get_reader(
    t2.select(
      t2.replica,
      t2.node)
    .where(t2.object_id == object_id));
  std::map<const_data_buffer, std::set<uint16_t>> replica_map;
  while(st.execute()) {
    replica_map[t2.node.get(st)].emplace(t2.replica.get(st));
  }

  //
  orm::sync_member_dbo t3;
  st = t.get_reader(
    t3.select(
      t3.member_node)
    .where(t3.object_id == object_id));
  std::set<const_data_buffer> members;
  while (st.execute()) {
    members.emplace(t2.node.get(st));
  }

  auto & client = *sp.get<dht::network::client>();
  client->send(
    sp,
    target_node,
    messages::sync_snapshot_response(
      object_id,
      client.current_node_id(),
      generation,
      current_term,
      commit_index,
      last_applied,
      replica_map,
      members));

}
