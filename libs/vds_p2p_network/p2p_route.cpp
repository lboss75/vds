#include "stdafx.h"
#include "p2p_route.h"
#include "private/p2p_route_p.h"
#include "p2p_node_info.h"

vds::p2p_route::p2p_route()
    : impl_(new _p2p_route()){

}

vds::p2p_route::~p2p_route() {
}

void
vds::p2p_route::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  this->impl_->random_broadcast(sp, message);
}


//////////////////////////////////////////////
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

int vds::_p2p_route::calc_distance(const guid & source_node, const guid & target_node)
{
  return 0;
}

void vds::_p2p_route::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  if(this->sessions_.empty()){
    throw std::runtime_error("No sessions");
  }

  auto index = std::rand() % this->sessions_.size();
  auto p = this->sessions_.begin();
  while(index != 0){
    ++p;
    --index;
  }

  p->second->send(sp, message);
}

void vds::_p2p_route::add(const service_provider &sp, const guid &partner_id,
                          const std::shared_ptr<udp_transport::_session> &session) {

  sp.get<logger>()->trace("P2PRoute", sp,"Established connection with %s", partner_id.str().c_str());
  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[partner_id] = std::make_shared<vds::_p2p_route::session>(session);
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

vds::async_task<> vds::_p2p_route::session::route(const vds::service_provider &sp, const vds::guid &node_id,
                                                  const vds::const_data_buffer &message) {
  throw std::runtime_error("Not implemented");
}

void vds::_p2p_route::session::close(
    const vds::service_provider &sp,
    const std::shared_ptr<std::exception> & ex) {
  this->target_->close(sp, ex);
}
