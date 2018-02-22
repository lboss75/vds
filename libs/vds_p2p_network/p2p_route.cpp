/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <messages/dht_find_node.h>
#include "stdafx.h"
#include "p2p_route.h"
#include "private/p2p_route_p.h"
#include "p2p_node_info.h"
#include "messages/p2p_message_id.h"
#include "messages/chunk_send_replica.h"
#include "db_model.h"
#include "chunk_replica_data_dbo.h"
#include "messages/chunk_offer_replica.h"
#include "messages/chunk_query_replica.h"
#include "run_configuration_dbo.h"
#include "private/p2p_crypto_tunnel_p.h"
#include "vds_debug.h"
#include "user_manager.h"
#include "messages/dht_ping.h"


vds::p2p_route::p2p_route()
: impl_(new _p2p_route()) {
}

vds::p2p_route::~p2p_route() {
}


//////////////////////////////////////////////
bool vds::_p2p_route::send(
    const service_provider &sp,
    const node_id_t & target_node_id,
    const const_data_buffer & message) {

  this->for_near(
      sp,
      target_node_id,
      1,
      [sp, message, target_node_id](
          const node_id_t & node_id,
          const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session) {
        proxy_session->send(
            sp,
            std::list<node_id_t>({node_id, target_node_id}),
            message);
      });
  return true;
}

void vds::_p2p_route::add_node(
    const vds::service_provider &sp,
    const node_id_t & id,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {
  auto index = this->current_node_id_.distance_exp(id);

  bucket * b;

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  auto p = this->buckets_.find(index);
  if(this->buckets_.end() == p){
    lock.unlock();

    std::unique_lock<std::shared_mutex> ulock(this->buckets_mutex_);
    auto p = this->buckets_.find(index);
    if(this->buckets_.end() == p) {
      b = &this->buckets_[index];
    }
    else {
      b = &p->second;
    }
  }
  else {
    b = &p->second;
    lock.unlock();
  }

  b->add_node(sp, id, proxy_session);
}

void vds::_p2p_route::on_timer(const vds::service_provider &sp) {
  this->ping_buckets(sp);
  this->update_route_table(sp);
}

void vds::_p2p_route::ping_buckets(const vds::service_provider &sp) {
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for(auto & p : this->buckets_){
    p.second.on_timer(sp);
  }
}

void vds::_p2p_route::search_nodes(
    const vds::service_provider &sp,
    const vds::node_id_t &target_id,
    size_t max_count,
    std::vector<vds::node_id_t> &result_nodes) {

  auto index = this->current_node_id_.distance_exp(target_id);

  std::map<vds::node_id_t, vds::node_id_t> result;
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for(
      uint8_t distance = 0;
      result_nodes.size() < max_count
      && (index + distance < 8 * node_id_t::SIZE || index - distance >= 0);
      ++distance){
    this->search_nodes(sp, target_id, result, index + distance);
    this->search_nodes(sp, target_id, result, index - distance);
  }

  for(auto & p : result){
    result_nodes.push_back(p.second);
    if(result_nodes.size() >= max_count){
      break;
    }
  }
}

void vds::_p2p_route::search_nodes(
    const vds::service_provider &sp,
    const vds::node_id_t &target_id,
    std::map<vds::node_id_t, vds::node_id_t> &result_nodes,
    uint8_t index) {
  auto p = this->buckets_.find(index);
  if(this->buckets_.end() == p) {
    return;
  }

  for(auto & node : p->second.nodes_){
    if(!node.is_good()){
      continue;
    }

    result_nodes[node.id_.distance(target_id)] = node.id_;
  }
}

void vds::_p2p_route::update_route_table(const vds::service_provider &sp) {

  for(uint8_t i = 0; i < 8 * node_id_t::SIZE; ++i){
    for(;;) {
      auto canditate = this->current_node_id_.generate_random_id(i);

      std::unique_lock<std::shared_mutex> lock(this->buckets_mutex_);
      auto p = this->buckets_.find(i);
      if(this->buckets_.end() == p || !p->second.contains(canditate)) {
        this->find_node(sp, canditate);
        break;
      }
    }
  }
}

void vds::_p2p_route::find_node(
    const vds::service_provider &sp,
    const vds::node_id_t &target_node_id) {
  this->for_near(
      sp,
      target_node_id,
      40,
      [sp, target_node_id](const node_id_t & node_id, const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session) {
        proxy_session->send(
            sp,
            std::list<node_id_t>({node_id}),
            p2p_messages::dht_find_node(target_node_id).serialize());

      });
}

void vds::_p2p_route::start(const vds::service_provider &sp) {
  auto user_mng = sp.get<user_manager>();
  asymmetric_private_key device_private_key;
  auto current_user = user_mng->get_current_device(sp, device_private_key);
  this->current_node_id_ = node_id_t(current_user.id());
}

void vds::_p2p_route::for_near(
    const vds::service_provider &sp,
    const vds::node_id_t &target_node_id,
    size_t max_count,
    const std::function<void(const node_id_t & node_id, const std::shared_ptr<vds::_p2p_crypto_tunnel> &)> &callback) {

  std::vector<node_id_t> result_nodes;
  this->search_nodes(sp, target_node_id, max_count, result_nodes);

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for(auto & node_id : result_nodes){
    auto index = this->current_node_id_.distance_exp(node_id);
    auto p = this->buckets_.find(index);
    if(this->buckets_.end() != p){
      std::shared_lock<std::shared_mutex> block(p->second.nodes_mutex_);
      for(auto & node : p->second.nodes_){
        if(node.id_ == node_id){
          callback(node_id, node.proxy_session_);
        }
      }
    }
  }
}

void vds::_p2p_route::stop(const vds::service_provider &sp) {

}

void vds::_p2p_route::close_session(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::_p2p_crypto_tunnel> &proxy_session) {

  std::unique_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for(auto & p : this->buckets_){
    auto n = p.second.nodes_.begin();
    while(p.second.nodes_.end() != n){
      if(n->proxy_session_.get() == proxy_session.get()){
        n = p.second.nodes_.erase(n);
      }
      else {
        ++n;
      }
    }
  }
}

void vds::_p2p_route::bucket::add_node(
    const vds::service_provider &sp,
    const node_id_t & id,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {

  std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
  if(MAX_NODES < this->nodes_.size()){
    this->nodes_.push_back(node(id, proxy_session));
    return;
  }

  for(auto & p : this->nodes_){
    if(!p.is_good()){
      p.reset(id, proxy_session);
      return;
    }
  }
}

void vds::_p2p_route::bucket::on_timer(const vds::service_provider &sp) {

  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for(auto & p : this->nodes_){
    p.proxy_session_->send(
        sp,
        std::list<node_id_t>({p.id_}),
        p2p_messages::dht_ping().serialize());
    p.pinged_++;
  }
}

bool vds::_p2p_route::bucket::contains(const vds::node_id_t &node_id) const {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for(auto & p : this->nodes_){
    if(p.id_ == node_id){
      return true;
    }
  }

  return false;
}
