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
#include "messages/sync_member_operation.h"
#include "messages/sync_new_election.h"
#include "messages/sync_coronation.h"
#include "sync_state_dbo.h"
#include "sync_member_dbo.h"
#include "messages/sync_apply_operation.h"
#include "messages/sync_leader_broadcast.h"
#include "sync_message_dbo.h"
#include "messages/sync_replica_operations.h"
#include "messages/sync_looking_storage.h"

void vds::dht::network::sync_process::query_unknown_records(const service_provider& sp, database_transaction& t) {
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

void vds::dht::network::sync_process::do_sync(
  const service_provider& sp,
  database_transaction& t) {

  this->sync_local_channels(sp, t);
  this->sync_replicas(sp, t);
  this->query_unknown_records(sp, t);
  this->sync_entries(sp, t);
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

void vds::dht::network::sync_process::apply_message(
    const service_provider& sp,
    database_transaction& t,
    const messages::transaction_log_request& message) {

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

void vds::dht::network::sync_process::apply_message(
  const service_provider& sp,
  database_transaction& t,
  const messages::sync_base_message_request & message) {

  orm::sync_state_dbo t1;
  orm::sync_member_dbo t2;
  auto st = t.get_reader(t1.select(
    t1.state, t1.generation, t1.current_term, t1.voted_for, t1.last_applied, t1.commit_index)
    .where(t1.object_id == base64::from_bytes(message.object_id())));

  if (st.execute()) {
    if (
      message.generation() < t1.generation.get(st)
      || (message.generation() == t1.generation.get(st) && message.current_term() < t1.current_term.get(st))) {
      this->send_coronation_request(sp, t, message.object_id(), message.leader_node());
    }
    else if(
      message.generation() == t1.generation.get(st)
      && message.generation() == t1.generation.get(st)
      && base64::to_bytes(t1.voted_for.get(st)) != message.leader_node()) {
        this->make_new_election(sp, t);
    }
  }
}

void vds::dht::network::sync_process::add_sync_entry(
  const service_provider& sp,
  database_transaction& t,
  const const_data_buffer& object_id) {

  auto & client = *sp.get<dht::network::client>();

  orm::sync_state_dbo t1;
  t.execute(t1.insert(
    t1.object_id = base64::from_bytes(object_id),
    t1.state = static_cast<uint8_t>(orm::sync_state_dbo::state_t::leader),
    t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
    t1.voted_for = base64::from_bytes(client.current_node_id()),
    t1.generation = 0,
    t1.current_term = 0,
    t1.commit_index = 0,
    t1.last_applied = 0));

  sp.get<logger>()->trace(ThisModule, sp, "Make leader %s:0:0", base64::from_bytes(object_id).c_str());
  std::map<const_data_buffer, std::set<uint16_t>> members_map;
  orm::sync_replica_map_dbo t2;
  for(uint16_t i = 0; i < _client::GENERATE_DISTRIBUTED_PIECES; ++i) {
    t.execute(t2.insert(
      t2.object_id = base64::from_bytes(object_id),
      t2.replica = i,
      t2.node = base64::from_bytes(client.current_node_id()),
      t2.last_access = std::chrono::system_clock::now()));

    members_map[client.current_node_id()].emplace(i);
  }
  client->send_near(
    sp,
    object_id,
    _client::GENERATE_DISTRIBUTED_PIECES,
    messages::sync_coronation_request(
      object_id,
      0,
      0,
      members_map,
      client.current_node_id()));

}


void vds::dht::network::sync_process::sync_entry::make_follower(
  const service_provider& sp,
  const const_data_buffer& object_id,
  const const_data_buffer& source_node,
  uint64_t current_term) {

  this->state_ = sync_entry::state_t::follower;
  this->current_term_ = current_term;
  this->voted_for_ = source_node;

  sp.get<logger>()->trace(ThisModule, sp, "Make follower %s:%d", base64::from_bytes(object_id).c_str(), this->current_term_);
}

void vds::dht::network::sync_process::sync_entry::make_leader(
  const service_provider& sp,
  const const_data_buffer& object_id) {
  vds_assert(sync_entry::state_t::canditate == this->state_);

  this->state_ = sync_entry::state_t::leader;
  this->last_operation_ = std::chrono::system_clock::now();

  sp.get<logger>()->trace(ThisModule, sp, "Make leader %s:%d", base64::from_bytes(object_id).c_str(), this->current_term_);

  auto & client = *sp.get<dht::network::client>();
  client->send_near(
    sp,
    object_id,
    _client::GENERATE_DISTRIBUTED_PIECES,
    messages::sync_coronation_request(
      object_id,
      this->current_term_,
      this->member_notes_,
      client.current_node_id()));

}

void vds::dht::network::sync_process::sync_entry::make_canditate(
  const service_provider& sp,
  const const_data_buffer& object_id) {

  this->state_ = sync_entry::state_t::canditate;
  this->last_operation_ = std::chrono::system_clock::now();
  this->current_term_++;
  this->member_notes_.clear();

  sp.get<logger>()->trace(ThisModule, sp, "Make candidate %s:%d", base64::from_bytes(object_id).c_str(), this->current_term_);

  auto & client = *sp.get<dht::network::client>();
  client->send_near(
    sp,
    object_id,
    _client::GENERATE_DISTRIBUTED_PIECES,
    messages::sync_new_election_request(
      object_id,
      this->current_term_,
      client.current_node_id()));
}


void vds::dht::network::sync_process::sync_entries(
  const service_provider& sp,
  database_transaction& t) {

  std::map<const_data_buffer, 
    std::tuple<
      orm::sync_state_dbo::state_t /*state*/,
      uint64_t /*generation*/,
      uint64_t /*current_term*/,
      uint64_t /*commit_index*/,
      uint64_t /*last_applied*/,
      uint64_t /*next_index*/,
      uint64_t /*object_size*/>> objects;
  orm::sync_state_dbo t1;
  auto st = t.get_reader(
    t1.select(
      t1.object_id,
      t1.object_size,
      t1.state,
      t1.generation,
      t1.current_term,
      t1.commit_index,
      t1.last_applied,
      t1.next_index)
    .where(t1.next_sync <= std::chrono::system_clock::now()));
  while (st.execute()) {
    const auto object_id = t1.object_id.get(st);
    auto & p = objects.at(base64::to_bytes(object_id));
    std::get<0>(p) = static_cast<orm::sync_state_dbo::state_t>(t1.state.get(st));
    std::get<1>(p) = t1.generation.get(st);
    std::get<2>(p) = t1.current_term.get(st);
    std::get<3>(p) = t1.commit_index.get(st);
    std::get<4>(p) = t1.last_applied.get(st);
    std::get<5>(p) = t1.next_index.get(st);
    std::get<6>(p) = t1.object_size.get(st);
  }

  for (auto & p : objects) {
    switch (std::get<0>(p.second)) {

    case orm::sync_state_dbo::state_t::follower: {
      this->make_candidate(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::canditate: {
      this->make_candidate(sp, t, p.first);
      break;
    }

    case orm::sync_state_dbo::state_t::leader: {
      auto & client = *sp.get<dht::network::client>();
      //Send leader broadcast
      orm::sync_member_dbo t2;
      st = t.get_reader(
        t2.select(t2.member_node, t2.last_activity)
        .where(t2.object_id == base64::from_bytes(p.first)));
      std::set<const_data_buffer> to_remove;
      std::set<const_data_buffer> member_nodes;
      while (st.execute()) {
        const auto member_node = t2.member_node.get(st);
        
        const auto last_activity = t2.last_activity.get(st);
        if(std::chrono::system_clock::now() - last_activity > MEMBER_TIMEOUT) {
          to_remove.emplace(base64::to_bytes(member_node));
        }
        else {
          member_nodes.emplace(base64::to_bytes(member_node));
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
              std::get<1>(p.second),
              std::get<2>(p.second),
              std::get<3>(p.second),
              std::get<4>(p.second)));
        }
      }
      else {
        //Remove members
        for (auto member_node : to_remove) {
          orm::sync_message_dbo t3;
          t.execute(
            t3.insert(
              t3.object_id = base64::from_bytes(p.first),
              t3.generation = std::get<1>(p.second),
              t3.current_term = std::get<2>(p.second),
              t3.index = std::get<5>(p.second)++,
              t3.message_type = static_cast<uint8_t>(orm::sync_message_dbo::message_type_t::remove_member),
              t3.member_node = base64::from_bytes(member_node)));

          for (auto member : member_nodes) {
            client->send(
              sp,
              member,
              messages::sync_replica_operations_request(
                p.first,
                client.current_node_id(),
                std::get<1>(p.second),
                std::get<2>(p.second),
                std::get<3>(p.second),
                std::get<4>(p.second),
                std::get<5>(p.second),
                orm::sync_message_dbo::message_type_t::remove_member,
                base64::from_bytes(member_node)));
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
            std::get<1>(p.second),
            std::get<2>(p.second),
            std::get<3>(p.second),
            std::get<4>(p.second),
            std::get<6>(p.second)),
          [member_nodes](const const_data_buffer & )->bool{
          });
      }

      t.execute(
        t1.update(
          t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
          t1.next_index = std::get<5>(p.second))
        .where(t1.object_id == base64::from_bytes(p.first)));

      break;
    }

    }
  }
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
    t1.object_id = base64::from_bytes(message.object_id()),
    t1.state = static_cast<uint8_t>(orm::sync_state_dbo::state_t::follower),
    t1.next_sync = std::chrono::system_clock::now() + LEADER_BROADCAST_TIMEOUT,
    t1.voted_for = base64::from_bytes(message.source_node()),
    t1.generation = message.generation(),
    t1.current_term = message.current_term(),
    t1.commit_index = 0,
    t1.last_applied = 0));

  orm::sync_replica_map_dbo t2;
  for(auto p : message.member_notes()) {
    for (auto replica : p.second) {
      t.execute(t2.insert(
        t2.object_id = base64::from_bytes(message.object_id()),
        t2.replica = replica,
        t2.node = base64::from_bytes(p.first),
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

        t.execute(t2.delete_if(t2.object_id == base64::from_bytes(message.object_id())));
        t.execute(t1.delete_if(t1.object_id == base64::from_bytes(message.object_id())));
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

void vds::dht::network::sync_process::sync_local_channels(
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

  if(leafs.empty()){
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

void vds::dht::network::sync_process::sync_replicas(
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


