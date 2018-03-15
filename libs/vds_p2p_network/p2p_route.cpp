/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

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
#include "p2p_network_statistic.h"
#include "messages/dht_find_node.h"


vds::p2p_route::p2p_route()
: impl_(new _p2p_route()) {
}

vds::p2p_route::~p2p_route() {
}


//////////////////////////////////////////////
bool vds::_p2p_route::send(
  const service_provider &sp,
  const node_id_t & target_node_id,
  const const_data_buffer & message,
  bool allow_skip) {
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);

  bool result = false;
  this->for_near(
    sp,
    target_node_id,
    1,
    [sp, allow_skip, message, target_node_id, &result, this](
      const node_id_t & node_id,
      const std::shared_ptr<vds::_p2p_crypto_tunnel> & proxy_session) {
    if (!allow_skip || node_id.distance(target_node_id) < this->current_node_id_.distance(target_node_id)) {
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
      vds_assert(allow_skip);

      sp.get<logger>()->trace(ThisModule, sp, "Don't send message %d bytes to %s over %s",
        message.size(),
        target_node_id.device_id().str().c_str(),
        proxy_session->address().to_string().c_str());
    }
  });
  return result;
}



void vds::_p2p_route::start(const vds::service_provider &sp) {
  auto user_mng = sp.get<user_manager>();
  asymmetric_private_key device_private_key;
  auto current_user = user_mng->get_current_device(sp, device_private_key);
  this->current_node_id_ = node_id_t(cert_control::get_id(current_user.user_certificate()));
  this->backgroud_timer_.start(sp, std::chrono::seconds(1), [sp, pthis = this->shared_from_this()]()->bool{
    pthis->on_timer(sp);
    return !sp.get_shutdown_event().is_shuting_down();
  });
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
    const std::set<uint16_t> &exist_replicas,
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

void vds::_p2p_route::get_statistic(const service_provider & sp, vds::p2p_network_statistic & result)
{
  result.this_node_ = this->current_node_id_.str();
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for (auto & p : this->buckets_) {
    p2p_network_statistic::bucket_info info;

    std::shared_lock<std::shared_mutex> block(p.second.nodes_mutex_);
    for(auto & node : p.second.nodes_) {
      auto state = string_format("%s -> %s", node.id_.str().c_str(), node.proxy_session_->address().to_string().c_str());
      if(!node.is_good()) {
        state += "[XXX]";
      }

      info.nodes_.push_back(state);
    }
    result.buckets_[p.first] = info;
  }
}

void vds::_p2p_route::apply(const service_provider& sp, const std::shared_ptr<_p2p_crypto_tunnel>& session,
  const p2p_messages::dht_pong& message) {

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  auto index = this->current_node_id_.distance_exp(message.source_node());
  auto p = this->buckets_.find(index);
  if (this->buckets_.end() != p) {
    std::shared_lock<std::shared_mutex> b_lock(p->second.nodes_mutex_);
    for (auto & node : p->second.nodes_) {
      if(node.id_ == message.source_node()) {
        node.pinged_ = 0;
      }
    }
  }
}


