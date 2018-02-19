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


vds::p2p_route::p2p_route()
: impl_(new _p2p_route()) {
}

vds::p2p_route::~p2p_route() {
}


bool
vds::p2p_route::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  return this->impl_->random_broadcast(sp, message);
}

//////////////////////////////////////////////

vds::async_task<> vds::_p2p_route::send_to(
    const service_provider &sp,
    const guid &node_id,
    const const_data_buffer &message) {

  const_data_buffer best_distance;
  std::shared_ptr<_p2p_crypto_tunnel> best_session;

  for(;;) {
    std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
    for (auto &p : this->sessions_) {
      const_data_buffer distance;
      if (!best_session || (best_distance > (distance = calc_distance(p.first, node_id)))) {
        best_distance = distance;
        best_session = p.second;
      }
    }

    if(best_session){
      break;
    }
  }

  throw std::runtime_error("Not implemented");
  //return best_session->send(sp, node_id, message);
}

bool vds::_p2p_route::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  if(this->sessions_.empty()){
    return false;
  }

  auto index = std::rand() % this->sessions_.size();
  auto p = this->sessions_.begin();
  while(index != 0){
    ++p;
    --index;
  }

  p->second->send(sp, message);
  return true;
}

void vds::_p2p_route::add(
	const service_provider &sp,
	const guid &partner_id,
  const std::shared_ptr<_p2p_crypto_tunnel> &session) {

  sp.get<logger>()->trace(ThisModule, sp,"Established connection with %s", partner_id.str().c_str());
  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[partner_id] = session;
}

std::set<vds::p2p::p2p_node_info> vds::_p2p_route::get_neighbors() const {
  std::set<p2p::p2p_node_info> result;

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for(auto p : this->sessions_) {
    result.emplace(p2p::p2p_node_info(p.first));
  }

  return result;
}

void vds::_p2p_route::broadcast(const vds::service_provider &sp, const vds::const_data_buffer &message) const {
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for(auto p : this->sessions_) {
    p.second->send(sp, message);
  }
}

bool vds::_p2p_route::send(
    const vds::service_provider &sp,
    const vds::guid &device_id,
    const vds::const_data_buffer &message) {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  auto p = this->sessions_.find(device_id);
  if(this->sessions_.end() == p) {
    return false;
  }

  p->second->send(sp, message);
  return true;
}

void vds::_p2p_route::close_session(
    const vds::service_provider &sp,
    const vds::guid &partner,
    const std::shared_ptr<std::exception> & ex) {

  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  auto p = this->sessions_.find(partner);
  if(this->sessions_.end() != p) {
    p->second->close(sp, ex);
  }
}

void vds::_p2p_route::query_replica(
    const service_provider &sp,
    const const_data_buffer & data_hash) {
  this->query_replica(
    sp,
    data_hash,
    sp.get_property<current_run_configuration>(service_provider::property_scope::any_scope)->id());
}

void vds::_p2p_route::query_replica(
  const service_provider &sp,
  const const_data_buffer & data_hash,
  const guid & source_node)
{
	std::shared_ptr<_p2p_crypto_tunnel> best_session;
	const_data_buffer best_distance;

	std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
	for(auto & p : this->sessions_)
	{
		const auto distance = calc_distance(p.first, data_hash);
		if(!best_session || best_distance > distance)
		{
			best_session = p.second;
			best_distance = distance;
		}
	}

	if (best_session) {
		best_session->send(
        sp,
        p2p_messages::chunk_query_replica(
          source_node,
          data_hash).serialize());
	}
}

vds::const_data_buffer vds::_p2p_route::calc_distance(
	const const_data_buffer& source_node,
	const const_data_buffer& target_node)
{
  vds_assert(source_node.size() == target_node.size());

  resizable_data_buffer result(source_node.size());

	for(size_t i = 0; i < source_node.size(); ++i)
	{
    result.data()[i] = (source_node.data()[i] ^ target_node.data()[i]);
	}

	return const_data_buffer(result.data(), result.size());
}

uint8_t vds::_p2p_route::calc_distance_exp(
    const vds::const_data_buffer &source_node,
    const vds::const_data_buffer &target_node) {
  vds_assert(source_node.size() == target_node.size());

  auto size = safe_cast<uint8_t>(source_node.size());
  for(uint8_t i = 0; i < size; ++i) {
    auto b = (source_node.data()[i] ^ target_node.data()[i]);
    if(0 == b){
      continue;
    }

    uint8_t result = (i << 3);

    uint8_t mask = 0x80;
    while(0 != mask){
      if(b >= mask){
        break;
      }

      ++result;
      mask >>= 1;
    }

    return result;
  }

  return 0;
}

void vds::_p2p_route::add_node(
    const vds::service_provider &sp,
    const node_id_t & id,
    const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session) {
  auto user_mng = sp.get<user_manager>();
  asymmetric_private_key device_private_key;
  auto current_user = user_mng->get_current_device(sp, device_private_key);

  auto index = node_id_t(current_user.id()).distance_exp(id);

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
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for(auto & p : this->buckets_){
    p.second.on_timer(sp);
  }
}

void vds::_p2p_route::search_nodes(
    const vds::service_provider &sp,
    const vds::const_data_buffer &target_id,
    size_t max_count,
    std::vector<vds::node_id_t> &result_nodes) {

  auto user_mng = sp.get<user_manager>();
  asymmetric_private_key device_private_key;
  auto current_user = user_mng->get_current_device(sp, device_private_key);
  auto index = node_id_t(current_user.id()).distance_exp(target_id);

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
    const vds::const_data_buffer &target_id,
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
      p.pinged_ = 0;
      return;
    }
  }
}

void vds::_p2p_route::bucket::on_timer(const vds::service_provider &sp) {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for(auto & p : this->nodes_){
    p.proxy_session_->send(sp, p2p_messages::dht_ping(p.id_).serialize());
    p.pinged_++;
  }
}
