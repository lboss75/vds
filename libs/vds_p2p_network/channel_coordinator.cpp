/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <messages/raft_request_vote.h>
#include <messages/raft_append_entries.h>
#include <messages/raft_vote_granted.h>
#include <db_model.h>
#include <raft_channel_member_dbo.h>
#include <raft_channel_record_dbo.h>
#include "stdafx.h"
#include "channel_coordinator.h"
#include "private/channel_coordinator_p.h"
#include "private/p2p_route_p.h"
#include "messages/raft_get_lead.h"
#include "messages/raft_current_lead.h"
#include "messages/raft_start_election.h"

void vds::channel_coordinator::apply(
    const vds::service_provider &sp,
    const std::shared_ptr<_p2p_route> & route,
    const vds::p2p_messages::raft_get_lead & messsage) {

  auto coordinator = this->get_or_create_coordinator(sp, route, messsage.channel_id());
  coordinator->apply(sp, route, messsage);
}

std::shared_ptr<vds::_channel_coordinator>
vds::channel_coordinator::get_or_create_coordinator(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route,
    const vds::guid &channel_id) {

  std::shared_lock<std::shared_mutex> rlock(this->coordinators_mutex_);
  auto p = this->coordinators_.find(channel_id);
  if(this->coordinators_.end() != p) {
    return p->second;
  }

  rlock.unlock();

  std::unique_lock<std::shared_mutex> wlock(this->coordinators_mutex_);
  p = this->coordinators_.find(channel_id);
  if(this->coordinators_.end() != p) {
    return p->second;
  }

  auto result = std::make_shared<_channel_coordinator>(channel_id);
  this->coordinators_[channel_id] = result;
  return result;
}

void vds::channel_coordinator::start(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {
  this->update_timer_.start(
      sp,
      std::chrono::milliseconds(100),
      [sp, route, pthis = this->shared_from_this()]() -> bool{
        std::shared_lock<std::shared_mutex> lock(pthis->coordinators_mutex_);
        for(auto & p : pthis->coordinators_){
          p.second->on_timer(sp, route);
        }

        return !sp.get_shutdown_event().is_shuting_down();
      });
}

////////////////////////////////////////////////////////////////////////
vds::_channel_coordinator::_channel_coordinator(
    const guid & channel_id)
: channel_id_(channel_id),
  state_(this_member_state_t::client),
  current_term_(0),
  timeout_elapsed_(0),
  request_timeout_(2),
  election_timeout_(10),
  election_timeout_rand_(10 + std::rand() % 10) {
}

void vds::_channel_coordinator::start(
    const vds::service_provider &sp,
    const std::shared_ptr<_p2p_route> & route) {
  std::shared_ptr<std::exception> error;
  barrier b;
  sp.get<db_model>()->async_transaction(sp,
                                        [pthis = this->shared_from_this()](vds::database_transaction & t)->bool{

    dbo::raft_channel_member_dbo t1;
    auto st = t.get_reader(
        t1
            .select(t1.device_id)
            .where(t1.channel_id == pthis->channel_id_));
    while(st.execute()){
      node_id_t node(t1.device_id.get(st));
      pthis->members_[node] = member_info_t(node);
    }

    dbo::raft_channel_record_dbo t2;
    st = t.get_reader(
        t2
            .select(db_max(t2.record_index))
            .where(t2.channel_id == pthis->channel_id_));

    if(st.execute()){
      uint64_t last_index;
      st.get_value(0, last_index);

      pthis->last_log_idx_ = last_index;
    }

    return true;
  }).execute([&b, &error](const std::shared_ptr<std::exception> & ex){
    if(ex){
      error = ex;
    }
    b.set();
  });

  b.wait();
  if(error){
    throw std::runtime_error(error->what());
  }
}

void vds::_channel_coordinator::on_timer(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {

  std::unique_lock<std::mutex> lock(this->state_mutex_);

  this->timeout_elapsed_++;

  if(1 == this->voted_for_me_.size()
     && !this->leader_){
    this->become_leader(sp, route);
  }

  //Send log
  if(this_member_state_t::leader == this->state_){
    if(this->request_timeout_ <= this->timeout_elapsed_) {
      for (auto & p : this->members_) {
        this->send_log(sp, route, p.second);
      }
    }
  }
  else
  if(this->election_timeout_rand_ <= this->timeout_elapsed_) {
    if(1 < this->voted_for_me_.size()
       && this->voted_for_me_.end() != this->voted_for_me_.find(route->current_node_id())){
      this->election_start(sp, route);
    }
  }

  if (this->last_applied_idx_ < this->commit_idx_) {
    this->apply_entry(sp, route);
  }
}


void vds::_channel_coordinator::apply(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route,
    const vds::p2p_messages::raft_get_lead &messsage) {
  std::unique_lock<std::mutex> lock(this->state_mutex_);
  if(!this->leader_){
    if(route->current_node_id() != messsage.sender_id()) {
      if(!this->leader_) {
        std::set<node_id_t> nodes;
        route->search_nodes(sp, this->channel_id_, 40, nodes);

        for (auto &p : nodes) {
          if (this->members_.end() == this->members_.find(p)) {
            this->members_.insert(p);
          }
        }

        for (auto &p : messsage.nodes()) {
          if (this->members_.end() == this->members_.find(p)) {
            this->members_.insert(p);
          }
        }

        if (this->members_.end() == this->members_.find(route->current_node_id())) {
          this->members_.insert(route->current_node_id());
        }

        while (40 < this->members_.size()) {
          this->members_.erase(std::prev(this->members_.end()));
        }

        if (this->members_.end() != this->members_.find(route->current_node_id())) {
          //This node is server
          for(auto & p : this->members_) {
            route->send(
                sp,
                p,
                p2p_messages::raft_start_election(
                    this->channel_id_,
                    messsage.sender_id(),
                    this->members_).serialize());
          }
        }
        else {
          //This node is client
          int count = 0;
          for(auto & p : this->members_) {
            route->send(
                sp,
                p,
                p2p_messages::raft_get_lead(
                    this->channel_id_,
                    messsage.sender_id(),
                    this->members_).serialize());
            if(3 < count++){
              break;
            }
          }
        }
      }
      else {
        route->send(
            sp,
            messsage.sender_id(),
            p2p_messages::raft_current_lead(
                this->channel_id_,
                this->leader_).serialize());
      }
    }
  }
  else {
    route->send(
        sp,
        messsage.sender_id(),
        p2p_messages::raft_current_lead(
            this->channel_id_,
            this->leader_).serialize());
  }
}

void vds::_channel_coordinator::become_candidate(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {

  this->current_term_++;
  this->leader_.clear();

  this->voted_for_me_.clear();
  this->voted_for_me_.insert(route->current_node_id());
  this->state_ = this_member_state_t::candidate;

  this->election_timeout_rand_ = this->election_timeout_ + std::rand() % this->election_timeout_;
  this->timeout_elapsed_ = 0;

  for (auto & p : this->members_) {
    if (p.first != route->current_node_id()) {
      route->send(
          sp,
          p.first,
          p2p_messages::raft_request_vote(
              this->channel_id_,
              this->current_term_,
              this->last_log_idx_,
              this->last_log_term_,
              route->current_node_id()).serialize());
    }
  }
}

void vds::_channel_coordinator::become_leader(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {

  sp.get<logger>()->debug(
      ThisModule,
      sp,
      "channel %s, becoming leader term %d",
      this->channel_id_.str().c_str(),
      this->current_term_);

  this->state_ = this_member_state_t::leader;
  this->timeout_elapsed_ = 0;
  for(auto & node : this->members_){
    this->send_log(sp, route, node.second);
  }
}


void vds::_channel_coordinator::send_log(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route,
    const vds::_channel_coordinator::member_info_t & node) {

  if(node.last_log_idx_ < this->last_log_idx_){
    std::list<const_data_buffer> records;
    for(size_t i = node.last_log_idx_ + 1; i < this->last_log_idx_; ++i){
      records.push_back(this->records_[i]);
    }
    route->send(
        sp,
        node.id_,
        p2p_messages::raft_append_entries(
            this->channel_id_,
            this->current_term_,
            this->commit_idx_,
            node.last_log_idx_ + 1,
            records).serialize());
  }
}

void
vds::_channel_coordinator::apply_entry(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {

  if(this->last_applied_idx_ == this->commit_idx_){
    return;
  }

  this->last_applied_idx_++;
  auto & data = this->records_[this->last_applied_idx_];
  binary_deserializer s(data);
  uint8_t command;

  s >> command;

  switch ((record_type_t)command){
    case record_type_t::normal:
      break;

    case record_type_t::add_client_node: {
      member_info_t node;
      s >> node.id_;

      node.last_log_idx_ = 0;
      node.last_log_term_ = 0;
      node.is_client_ = true;

      this->members_[node.id_] = node;
      break;
    }

    case record_type_t::add_server_node:{
      member_info_t node;
      s >> node.id_;

      node.last_log_idx_ = 0;
      node.last_log_term_ = 0;
      node.is_client_ = false;

      this->members_[node.id_] = node;
      break;
    }

    case record_type_t::remove_node:
      break;

    default:
      throw std::runtime_error("Invalid value");
  }
}

void vds::_channel_coordinator::election_start(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route) {
  this->become_candidate(sp, route);
}

void vds::_channel_coordinator::apply(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route,
    const vds::p2p_messages::raft_request_vote & messsage) {

  std::unique_lock<std::mutex> lock(this->state_mutex_);
  if(this->members_.end() == this->members_.find(messsage.node_id())){
    sp.get<logger>()->debug(
        ThisModule,
        sp,
        "Request vote from unknown node %s",
        messsage.node_id().device_id().str().c_str());
    return;
  }

  if(this->current_term_ < messsage.current_term()){
    this->current_term_ = messsage.current_term();
    if(this_member_state_t::client != this->state_){
      this->state_ = this_member_state_t::follower;
      this->leader_.clear();
    }
  }

  if(
    !this->voted_for_
    && this->current_term_ == messsage.current_term()
    && this->voted_for_me_.end() != this->voted_for_me_.find(route->current_node_id())
    && !this->records_.empty()){
    auto p = this->records_.rbegin();

    if(p->second.term_ < messsage.current_term()
        || (p->second.term_ == messsage.current_term() && p->first <= messsage.last_log_idx())){
      this->voted_for_ = messsage.node_id();
      this->leader_.clear();
      this->timeout_elapsed_ = 0;

      route->send(
          sp,
          messsage.node_id(),
          p2p_messages::raft_vote_granted(
              this->channel_id_,
              messsage.current_term(),
              route->current_node_id()).serialize());
    }
  }
}

void vds::_channel_coordinator::apply(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_route> &route,
    const vds::p2p_messages::raft_vote_granted & messsage) {
  std::unique_lock<std::mutex> lock(this->state_mutex_);
  if(this_member_state_t::candidate != this->state_){
    return;
  }

  auto p = this->members_.find(messsage.node_id());
  if(this->members_.end() == p || p->second.is_client_){
    return;
  }

  if(this->current_term_ < messsage.current_term()){
    this->state_ = this_member_state_t::follower;
    this->leader_.clear();
    return;
  }

  if(this->current_term_ > messsage.current_term()){
    return;
  }

  this->voted_for_me_.insert(p->first);

  size_t server_count = 0;
  for(auto & member : this->members_){
    if(!member.second.is_client_){
      ++server_count;
    }
  }

  if(this->voted_for_me_.size() > 1 + server_count / 2) {
    this->become_leader(sp, route);
  }
}

