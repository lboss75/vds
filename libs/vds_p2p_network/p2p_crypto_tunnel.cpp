//
// Created by vadim on 12.11.17.
//

#include "p2p_crypto_tunnel.h"
#include "private/p2p_crypto_tunnel_p.h"

void
vds::p2p_crypto_tunnel::start(
    const vds::udp_transport::session &session,
    const std::string &login,
    const std::string &password) {

  this->impl_.reset(new _p2p_crypto_tunnel(session));

}

void vds::p2p_crypto_tunnel::handle_incoming_message(
    const service_provider &sp,
    const vds::const_data_buffer &message) {

  this->impl_->process_input_command(sp, message);

}

void vds::p2p_crypto_tunnel::start(
    const vds::udp_transport::session &session,
    const class certificate &node_cert,
    const class asymmetric_private_key &node_key){

   std::list<certificate> certificate_chain;
  certificate_chain.push_back(node_cert);

  this->impl_.reset(new _p2p_crypto_tunnel(session, certificate_chain, node_key));
}
