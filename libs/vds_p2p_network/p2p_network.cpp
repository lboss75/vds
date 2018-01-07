#include "stdafx.h"
#include "p2p_network.h"
#include "private/p2p_network_p.h"
#include "ip2p_network_client.h"
#include "udp_socket.h"
#include "private/p2p_crypto_tunnel_p.h"
#include "private/p2p_crypto_tunnel_with_login_p.h"
#include "private/p2p_crypto_tunnel_with_certificate_p.h"
#include "private/p2p_route_p.h"
#include "run_configuration_dbo.h"
#include "certificate_private_key_dbo.h"
#include "p2p_network_service.h"
#include "private/p2p_network_service_p.h"

vds::p2p_network::p2p_network()
:impl_(new _p2p_network()){
}

vds::p2p_network::~p2p_network() {

}


vds::async_task<>
vds::p2p_network::random_broadcast(const vds::service_provider &sp, const vds::const_data_buffer &message) {
  return this->impl_->random_broadcast(sp, message);
}

vds::async_task<> vds::p2p_network::init_server(
    const vds::service_provider &sp,
    const std::string &user_name,
    const std::string &user_password,
    const std::string &device_name,
    int port) {
  return this->impl_->init_server(sp, user_name, user_password, device_name, port);
}

vds::async_task<> vds::p2p_network::start_network(const vds::service_provider &sp) {
  return this->impl_->start_network(sp);
}

vds::async_task<> vds::p2p_network::prepare_to_stop(const vds::service_provider &sp) {
  return this->impl_->prepare_to_stop(sp);
}

void vds::p2p_network::stop(const vds::service_provider &sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}

//////////////////////////////////
vds::_p2p_network::_p2p_network()
{
  this->leak_detect_.name_ = "_p2p_network";
  this->leak_detect_.dump_callback_ = [this](leak_detect_collector * collector){
    for(auto & p : this->network_services_){
      collector->add(p);
    }
  };
}

vds::_p2p_network::~_p2p_network() {

}

vds::async_task<>
vds::_p2p_network::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  return this->route_.random_broadcast(sp, message);
}

void vds::_p2p_network::add_route(const service_provider &sp, const vds::guid &partner_id,
                                  const std::shared_ptr<vds::udp_transport::_session> &session) {
  this->route_->add(sp, partner_id, session);
}


vds::async_task<> vds::_p2p_network::init_server(
    const vds::service_provider &sp,
    const std::string &user_name,
    const std::string &user_password,
    const std::string &device_name,
    int port) {
  this->network_services_.push_back(p2p_network_service());
  return this->network_services_.rbegin()->start(sp, device_name, port, user_name, user_password);
}

struct run_data
{
  int port;
  std::list<vds::certificate> cert_chain;
  vds::asymmetric_private_key key;
};


vds::async_task<> vds::_p2p_network::start_network(const vds::service_provider &sp) {

  imt_service::enable_async(sp);
  auto run_conf = std::make_shared<std::list<run_data>>();
  return sp.get<db_model>()->async_transaction(sp, [run_conf](database_transaction & t){
    run_configuration_dbo t1;
    certificate_dbo t2;
    auto st = t.get_reader(
        t1.select(t1.port, t2.cert, t1.cert_private_key)
            .inner_join(t2, t2.id == t1.cert_id));
    while(st.execute()){
      run_data item;
      item.port = t1.port.get(st);
      item.key = asymmetric_private_key::parse_der(t1.cert_private_key.get(st), std::string());
      item.cert_chain.push_back(certificate::parse_der(t2.cert.get(st)));

      run_conf->push_back(item);
    }

    for(auto & conf : *run_conf) {
      auto parent_id = cert_control::get_parent_id(conf.cert_chain.front());
      while(parent_id){
        certificate_dbo t4;
        auto st = t.get_reader(t4.select(t4.cert).where(t4.id == parent_id));
        if(!st.execute()){
          throw std::runtime_error("Invalid certificate ID " + parent_id.str());
        }

        auto cert = certificate::parse_der(t4.cert.get(st));
        conf.cert_chain.push_front(cert);
        parent_id = cert_control::get_parent_id(cert);
      }
    }

    return true;
  }).then([sp, run_conf, pthis = this->shared_from_this()](){
    if(run_conf->empty()){
      throw std::runtime_error("There is no active network configuration");
    }

    auto result = async_task<>::empty();
    for(auto & conf : *run_conf) {
      pthis->network_services_.push_back(p2p_network_service());
      result = result.then([pthis, sp, conf]() {
        return pthis->network_services_.rbegin()->start(
            sp, conf.port, conf.cert_chain, conf.key);
      });
    }
    return result;
  });
}

vds::async_task<> vds::_p2p_network::prepare_to_stop(const vds::service_provider &sp) {
  if(this->network_services_.empty()){
    return async_task<>::empty();
  }

  return [pthis = this->shared_from_this(), sp](const async_result<> & result) {
    auto runner = new _async_series(result, pthis->network_services_.size());
    for(auto & p : pthis->network_services_){
      runner->add(p.prepare_to_stop(sp));
    }
  };
}

void vds::_p2p_network::stop(const vds::service_provider &sp) {
  for(auto & p : this->network_services_){
    p.stop(sp);
  }
  this->network_services_.clear();
}
