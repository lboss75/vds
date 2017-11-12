#include "stdafx.h"
#include "p2p_network.h"
#include "private/p2p_network_p.h"

vds::p2p_network::p2p_network() {

}

vds::p2p_network::~p2p_network() {

}

void
vds::p2p_network::start(
    const vds::service_provider &sp,
    const std::shared_ptr<class ip2p_network_storage> &storage,
    const std::shared_ptr<class ip2p_network_client> &client,
    const std::string &login,
    const std::string &password) {

  this->impl_.reset(new _p2p_network(storage, client));
  this->impl_->start(sp, login, password);

}

void
vds::p2p_network::start(
    const vds::service_provider &sp,
    const std::shared_ptr<class ip2p_network_storage> &storage,
    const std::shared_ptr<class ip2p_network_client> &client,
    const vds::certificate &node_cert,
    const vds::asymmetric_private_key &node_key) {

  this->impl_.reset(new _p2p_network(storage, client));
  this->impl_->start(sp, node_cert, node_key);
}
