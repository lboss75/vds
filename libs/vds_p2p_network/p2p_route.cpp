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
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);

  bool result = false;
  this->for_near(
    sp,
    target_node_id,
    1,
    [sp, message, target_node_id, &result, this](
      const node_id_t & node_id,
      const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session) {
    if (node_id.distance(target_node_id) < this->current_node_id_.distance(target_node_id)) {
      sp.get<logger>()->trace(ThisModule, sp, "Send message %d bytes to %s over %s becouse node %s is near node %s for target %s",
        message.size(),
        target_node_id.device_id().str().c_str(),
        proxy_session->address().to_string().c_str(),
        node_id.str().c_str(),
        this->current_node_id_.str().c_str(),
        target_node_id.str().c_str());
      proxy_session->send(
        sp,
        target_node_id,
        message);
      result = true;
    }
    else {
      sp.get<logger>()->trace(ThisModule, sp, "Don't send message %d bytes to %s over %s",
        message.size(),
        target_node_id.device_id().str().c_str(),
        proxy_session->address().to_string().c_str());
    }
  });
  return result;
}

void vds::_p2p_route::add_node(
    const vds::service_provider &sp,
    const node_id_t & id,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {
  sp.get<logger>()->trace(
      ThisModule,
      sp,
      "Add route to %s over %s",
      id.device_id().str().c_str(),
      proxy_session->address().to_string().c_str());

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
  std::list<node> &result_nodes) {
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  this->_search_nodes(sp, target_id, max_count, result_nodes);
}

void vds::_p2p_route::_search_nodes(
  const vds::service_provider &sp,
  const vds::node_id_t &target_id,
  size_t max_count,
  std::list<node> &result_nodes) {

  auto index = this->current_node_id_.distance_exp(target_id);

  std::map<vds::node_id_t, node> result;
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
    std::map<vds::node_id_t, node> &result_nodes,
    uint8_t index) {
  auto p = this->buckets_.find(index);
  if(this->buckets_.end() == p) {
    return;
  }

  for(auto & node : p->second.nodes_){
    if(!node.is_good()){
      continue;
    }

    result_nodes[node.id_.distance(target_id)] = node;
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
    sp.get<logger>()->trace(ThisModule, sp, "DHT find node %s over %s (%s)",
      target_node_id.device_id().str().c_str(),
      node_id.device_id().str().c_str(),
      proxy_session->address().to_string().c_str());
        proxy_session->send(
            sp,
            node_id,
            p2p_messages::dht_find_node(target_node_id).serialize());

      });
}

vds::_p2p_route::_p2p_route()
: backgroud_timer_("P2P Route timer"){
}

void vds::_p2p_route::start(const vds::service_provider &sp) {
  auto user_mng = sp.get<user_manager>();
  asymmetric_private_key device_private_key;
  auto current_user = user_mng->get_current_device(sp, device_private_key);
  this->current_node_id_ = node_id_t(current_user.id());
  this->backgroud_timer_.start(sp, std::chrono::seconds(1), [sp, pthis = this->shared_from_this()]()->bool{
    pthis->on_timer(sp);
    return !sp.get_shutdown_event().is_shuting_down();
  });
}

void vds::_p2p_route::for_near(
    const vds::service_provider &sp,
    const vds::node_id_t &target_node_id,
    size_t max_count,
    const std::function<void(const node_id_t & node_id, const std::shared_ptr<vds::_p2p_crypto_tunnel> &)> &callback) {

  std::list<node> result_nodes;
  this->_search_nodes(sp, target_node_id, max_count, result_nodes);

  for(auto & node : result_nodes){
    auto index = this->current_node_id_.distance_exp(node.id_);
    auto p = this->buckets_.find(index);
    if(this->buckets_.end() != p){
      std::shared_lock<std::shared_mutex> block(p->second.nodes_mutex_);
      for(auto & pnode : p->second.nodes_){
        if(pnode.id_ == node.id_){
          callback(node.id_, pnode.proxy_session_);
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

void vds::_p2p_route::query_replica(
    const vds::service_provider &sp,
    const vds::const_data_buffer &data_id,
    const std::vector<uint16_t> &exist_replicas,
    uint16_t distance) {

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  this->for_near(
      sp,
      node_id_t(data_id.data(), data_id.size()),
      distance,
      [
          sp,
          message = p2p_messages::chunk_query_replica(
              this->current_node_id_.device_id(),
              data_id,
              exist_replicas).serialize()]
          (
              const node_id_t & node_id,
              const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session){
        proxy_session->send(
            sp,
            node_id,
            message);
      });
}

void vds::_p2p_route::bucket::add_node(
    const vds::service_provider &sp,
    const node_id_t & id,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {

  std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
  if(MAX_NODES > this->nodes_.size()){
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
        p.id_,
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
