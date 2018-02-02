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

vds::p2p_route::p2p_route() {
}

vds::p2p_route::~p2p_route() {
}

void vds::p2p_route::init(
  const guid & this_device_id) {
  this->impl_.reset(new _p2p_route(this_device_id));  
}


bool
vds::p2p_route::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  return this->impl_->random_broadcast(sp, message);
}

//////////////////////////////////////////////
vds::_p2p_route::_p2p_route(const guid & this_device_id)
  : this_device_id_(this_device_id) {
}


vds::async_task<> vds::_p2p_route::send_to(const service_provider &sp, const guid &node_id,
                                           const const_data_buffer &message) {

  int best_distance;
  std::shared_ptr<session> best_session;

  for(;;) {
    std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
    for (auto &p : this->sessions_) {
      int distance;
      if (!best_session || (best_distance > (distance = this->calc_distance(p.first, node_id)))) {
        p.second->lock();
        if (p.second->is_busy()) {
          p.second->unlock();
          continue;
        }

        best_distance = distance;
        if (best_session) {
          best_session->unlock();
        }

        best_session = p.second;
      }
    }

    if(best_session){
      break;
    }
  }

  return best_session->route(sp, node_id, message);
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
  const std::shared_ptr<udp_transport::_session> &session) {

  sp.get<logger>()->trace(ThisModule, sp,"Established connection with %s", partner_id.str().c_str());
  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[partner_id] = std::make_shared<vds::_p2p_route::session>(session);
  
  sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), partner_id, session](database_transaction & t) {
    dbo::chunk_replica_data_dbo t1;
    auto st = t.get_reader(t1.select(t1.replica_hash).order_by(db_desc_order(t1.distance)));
    while(st.execute()) {
      auto replica_hash = t1.replica_hash.get(st);
      if(calc_distance(pthis->this_device_id_, replica_hash) > calc_distance(partner_id, replica_hash)) {
        session->send(
          sp,
          p2p_messages::chunk_offer_replica(replica_hash).serialize());
      }
    }
  })
  .execute([sp](const std::shared_ptr<std::exception> & ex) {
    if (ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at find better place to chunk", ex->what());
    }
  });
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
    const const_data_buffer & data_hash)
{
	std::shared_ptr<session> best_session;
	size_t best_distance;

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
            this->this_device_id_,
            data_hash).serialize());
	}
}


void vds::_p2p_route::session::lock() {
  this->state_mutex_.lock();
}

void vds::_p2p_route::session::unlock() {
  this->state_mutex_.unlock();
}

void
vds::_p2p_route::session::send(const vds::service_provider &sp, const vds::const_data_buffer &message) {
  this->target_->send(sp, message);
}

vds::async_task<> vds::_p2p_route::session::route(
  const vds::service_provider &sp,
  const vds::guid &node_id,
  const vds::const_data_buffer &message) {

  throw std::runtime_error("Not implemented");
}

void vds::_p2p_route::session::close(
    const vds::service_provider &sp,
    const std::shared_ptr<std::exception> & ex) {
  this->target_->close(sp, ex);
}

size_t vds::_p2p_route::calc_distance(
	const const_data_buffer& source_node,
	const const_data_buffer& target_node)
{
	size_t result = 0;

	const size_t min_size = (source_node.size() < target_node.size()) ? source_node.size() : target_node.size();
	for(size_t i = 0; i < min_size; ++i)
	{
		const uint8_t value = (source_node.data()[i] ^ target_node.data()[i]);
		if(0 != value)
		{
			uint8_t mask = 1;
			while(mask != 0)
			{
				if(0 != (mask & value))
				{
					result++;
				}

				mask <<= 1;
			}
		}
	}

	return result + 8 * (((source_node.size() > target_node.size()) ? source_node.size() : target_node.size()) - min_size);
}
