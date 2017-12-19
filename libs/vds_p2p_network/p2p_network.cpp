#include "stdafx.h"
#include "p2p_network.h"
#include "private/p2p_network_p.h"
#include "ip2p_network_client.h"
#include "udp_socket.h"
#include "private/p2p_crypto_tunnel_p.h"
#include "private/p2p_crypto_tunnel_with_login_p.h"
#include "private/p2p_crypto_tunnel_with_certificate_p.h"
#include "private/p2p_route_p.h"

vds::p2p_network::p2p_network() {

}

vds::p2p_network::~p2p_network() {

}


vds::async_task<>
vds::p2p_network::random_broadcast(const vds::service_provider &sp, const vds::const_data_buffer &message) {
  return this->impl_->random_broadcast(sp, message);
}


//////////////////////////////////
vds::_p2p_network::_p2p_network(
    const std::shared_ptr<ip2p_network_client> &client)
: client_(client) {
}

vds::_p2p_network::~_p2p_network() {

}

vds::async_task<>
vds::_p2p_network::random_broadcast(
    const vds::service_provider &sp,
    const vds::const_data_buffer &message) {
  return this->route_.random_broadcast(sp, message);
}

void vds::_p2p_network::add_route(
    const vds::guid &partner_id,
    const vds::guid &this_node_id,
    const std::shared_ptr<vds::udp_transport::_session> &session) {
  this->route_->add(partner_id, this_node_id, session);

}

