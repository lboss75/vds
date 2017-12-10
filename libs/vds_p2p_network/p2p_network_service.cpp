//
// Created by vadim on 30.10.17.
//

#include "p2p_network_service.h"
#include "private/p2p_network_service_p.h"
#include "ip2p_network_client.h"
#include "stdafx.h"


vds::async_task<> vds::p2p_network_service::start(
    const vds::service_provider &sp,
    int port,
    const std::list<certificate> &certificate_chain,
    const vds::asymmetric_private_key &node_key) {
  this->impl_.reset(new _p2p_network_service(sp));
  return this->impl_->start(sp, port, certificate_chain, node_key);

}

vds::async_task<> vds::p2p_network_service::start(
    const vds::service_provider &sp,
    int port,
    const std::string &login,
    const std::string &password) {
  this->impl_.reset(new _p2p_network_service(sp));
  return this->impl_->start(sp, port, login, password);
}
//////////////////////////
vds::_p2p_network_service::_p2p_network_service(const vds::service_provider &sp)
: network_(new _p2p_network(
    sp.get<ip2p_network_client>()->shared_from_this()))
{
}

vds::async_task<>
vds::_p2p_network_service::start(
    const vds::service_provider &sp,
    int port,
    const std::string &login,
    const std::string &password) {
  return this->network_->start(sp, port, login, password);
}

vds::async_task<> vds::_p2p_network_service::start(const vds::service_provider &sp, int port,
                                                   const std::list<certificate> &certificate_chain,
                                                   const asymmetric_private_key &node_key) {
  return this->network_->start(sp, port, certificate_chain, node_key);
}
