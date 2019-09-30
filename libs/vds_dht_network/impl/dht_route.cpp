/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_route.h"
#include "route_statistic.h"

vds::dht::dht_route::node::node()
  : pinged_(0) {
}

vds::dht::dht_route::node::node(const const_data_buffer& id, const session_type& proxy_session, uint8_t hops)
  : node_id_(id),
  proxy_session_(proxy_session),
  hops_(hops),
  pinged_(0) {
}

vds::dht::dht_route::node::node(node&& origin)
  : node_id_(std::move(origin.node_id_)),
  proxy_session_(std::move(origin.proxy_session_)),
  hops_(origin.hops_),
  pinged_(origin.pinged_) {
}

bool vds::dht::dht_route::node::is_good() const {
  return this->pinged_ < 10;
}

void vds::dht::dht_route::node::reset(const const_data_buffer& id, const session_type& proxy_session, uint8_t hops) {
  this->node_id_ = id;
  this->proxy_session_ = proxy_session;
  this->hops_ = hops;
  this->pinged_ = 0;
}

bool vds::dht::dht_route::bucket::add_node(const const_data_buffer& id, const session_type& proxy_session, uint8_t hops, bool allow_skip) {

  std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
  for (const auto& p : this->nodes_) {
    if (p->node_id_ == id && p->proxy_session_->address() == proxy_session->address()) {
      return false;//Already exists
    }
  }

  if (!allow_skip || 0 == hops || MAX_NODES > this->nodes_.size()) {
    this->nodes_.push_back(std::make_shared<node>(id, proxy_session, hops));
    return true;
  }

  for (auto& p : this->nodes_) {
    if (!p->is_good()) {
      p->reset(id, proxy_session, hops);
      return true;
    }
  }

  return false;
}

void vds::dht::dht_route::bucket::remove_session(const session_type& proxy_session) {
  std::unique_lock<std::shared_mutex> ulock(this->nodes_mutex_);
  for (auto p = this->nodes_.begin(); p != this->nodes_.end(); ) {
    if ((*p)->proxy_session_->address() == proxy_session->address()) {
      p = this->nodes_.erase(p);
    }
    else {
      ++p;
    }
  }
}

vds::async_task<vds::expected<void>> vds::dht::dht_route::bucket::on_timer(
  const service_provider* sp,
  const dht_route* owner,
  const std::shared_ptr<network::iudp_transport> & transport) {
  std::list<std::tuple<const_data_buffer, session_type>> sessions;

  this->nodes_mutex_.lock();
  for (auto p : this->nodes_) {
    sp->get<logger>()->trace("DHT", "Bucket node node_id=%s,proxy_session=%s,pinged=%d,hops=%d",
      base64::from_bytes(p->node_id_).c_str(),
      p->proxy_session_->address().to_string().c_str(),
      p->pinged_,
      p->hops_);

    p->pinged_++;
    sessions.push_back({ p->node_id_, p->proxy_session_ });
  }
  this->nodes_mutex_.unlock();

  for (const auto& s : sessions) {
    CHECK_EXPECTED_ASYNC(co_await std::get<1>(s)->ping_node(
      std::get<0>(s),
      transport));
  }
  co_return expected<void>();
}

bool vds::dht::dht_route::bucket::contains(const const_data_buffer& node_id) const {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for (auto& p : this->nodes_) {
    if (p->node_id_ == node_id) {
      return true;
    }
  }

  return false;
}

void vds::dht::dht_route::bucket::mark_pinged(const const_data_buffer& target_node, const network_address& address) {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for (auto& p : this->nodes_) {
    if (p->node_id_ == target_node && p->proxy_session_->address() == address) {
      p->pinged_ = 0;
      break;
    }
  }
}

void vds::dht::dht_route::bucket::get_statistics(route_statistic& result) {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for (auto& p : this->nodes_) {
    result.items_.push_back(
      route_statistic::route_info {
        p->node_id_,
        p->proxy_session_->address().to_string(),
        p->pinged_,
        p->hops_
      }
    );
  }
}

void vds::dht::dht_route::bucket::get_neighbors(std::list<std::shared_ptr<node>>& result_nodes) const {
  std::shared_lock<std::shared_mutex> lock(this->nodes_mutex_);
  for (auto& p : this->nodes_) {
    if (p->hops_ == 0) {
      result_nodes.push_back(p);
    }
  }
}

vds::expected<size_t> vds::dht::dht_route::looking_nodes(const const_data_buffer& target_id, const lambda_holder_t<expected<bool>, const node&>& filter, std::map<const_data_buffer, std::map<const_data_buffer, std::shared_ptr<node>>>& result_nodes, size_t index) const {
  size_t result = 0;
  auto p = this->buckets_.find(index);
  if (this->buckets_.end() == p) {
    return result;
  }

  std::shared_lock<std::shared_mutex> lock(p->second->nodes_mutex_);

  for (const auto& node : p->second->nodes_) {
    if (!node->is_good()) {
      continue;
    }

    GET_EXPECTED(filter_result, filter(*node));
    if (!filter_result) {
      continue;;
    }

    auto& result_node = result_nodes[dht_object_id::distance(node->node_id_, target_id)];
    auto exists = result_node.find(node->node_id_);
    if (result_node.end() == exists) {
      result_node[node->node_id_] = node;
      ++result;
    }
    else {
      if (exists->second->hops_ > node->hops_) {
        exists->second = node;
      }
    }
  }

  return result;
}

vds::async_task<vds::expected<void>> vds::dht::dht_route::ping_buckets(std::shared_ptr<network::iudp_transport> transport) {
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for (auto& p : this->buckets_) {
    logger::get(this->sp_)->trace("DHT", "Bucket %d", p.first);
    CHECK_EXPECTED_ASYNC(co_await p.second->on_timer(this->sp_, this, transport));
  }

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::dht_route::on_timer(std::shared_ptr<network::iudp_transport> transport) {
  return this->ping_buckets(std::move(transport));
}

vds::expected<void> vds::dht::dht_route::search_nodes(
  const const_data_buffer& target_id,
  size_t max_count,
  const std::function<expected<bool>(const node & node)>& filter,
  std::map<const_data_buffer, std::map<const_data_buffer, std::shared_ptr<node>>>& result_nodes) const
{
  if (this->buckets_.empty()) {
    return expected<void>();
  }

  auto index = dht_object_id::distance_exp(this->current_node_id_, target_id);

  auto min_index = this->buckets_.begin()->first;
  auto max_index = this->buckets_.rbegin()->first;

  size_t count = 0;
  for (
    size_t distance = 0;
    result_nodes.size() < max_count
    && (index + distance <= max_index || (index >= distance && index - distance >= min_index));
    ++distance) {
    if (index + distance <= max_index) {
      GET_EXPECTED(found_nodes, this->looking_nodes(target_id, filter, result_nodes, index + distance));
      count += found_nodes;
    }
    if (index >= distance && index - distance >= min_index) {
      GET_EXPECTED(found_nodes, this->looking_nodes(target_id, filter, result_nodes, index - distance));
      count += found_nodes;
    }
    if (count > max_count) {
      break;
    }
  }

  return expected<void>();
}

void vds::dht::dht_route::get_neighbors(std::list<std::shared_ptr<node>>& result_nodes) const
{
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for (auto& p : this->buckets_) {
    p.second->get_neighbors(result_nodes);
  }
}

void vds::dht::dht_route::mark_pinged(const const_data_buffer& target_node, const network_address& address)
{
  auto index = dht_object_id::distance_exp(this->current_node_id_, target_node);

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  auto p = this->buckets_.find(index);
  if (this->buckets_.end() != p) {
    p->second->mark_pinged(target_node, address);
  }
}

void vds::dht::dht_route::get_statistics(route_statistic& result)
{
  result.node_id_ = this->current_node_id();
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for (auto& p : this->buckets_) {
    p.second->get_statistics(result);
  }
}

void vds::dht::dht_route::remove_session(const session_type& session)
{
  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  for (auto p : this->buckets_) {
    p.second->remove_session(session);
  }
}

vds::dht::dht_route::dht_route(
  const service_provider* sp,
  const const_data_buffer& this_node_id)
  : sp_(sp), current_node_id_(this_node_id) {
}

const vds::const_data_buffer& vds::dht::dht_route::current_node_id() const
{
  return this->current_node_id_;
}

bool vds::dht::dht_route::add_node(const const_data_buffer& id, const session_type& proxy_session, uint8_t hops, bool allow_skip)
{
  vds_assert(id != this->current_node_id_);
  vds_assert(proxy_session->partner_node_id() != this->current_node_id_);

  const auto index = dht_object_id::distance_exp(this->current_node_id_, id);
  std::shared_ptr<bucket> b;

  std::shared_lock<std::shared_mutex> lock(this->buckets_mutex_);
  auto p = this->buckets_.find(index);
  if (this->buckets_.end() == p) {
    lock.unlock();

    std::unique_lock<std::shared_mutex> ulock(this->buckets_mutex_);
    auto p = this->buckets_.find(index);
    if (this->buckets_.end() == p) {
      b = std::make_shared<bucket>();
      this->buckets_[index] = b;
    }
    else {
      b = p->second;
    }
  }
  else {
    b = p->second;
    lock.unlock();
  }

  return b->add_node(id, proxy_session, hops, allow_skip);
}
