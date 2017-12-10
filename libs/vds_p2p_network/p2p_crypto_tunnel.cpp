//
// Created by vadim on 12.11.17.
//

#include "p2p_crypto_tunnel.h"
#include "private/p2p_crypto_tunnel_p.h"

void vds::p2p_crypto_tunnel::start(
    const service_provider & sp) {

  static_cast<_p2p_crypto_tunnel *>(this->impl_.get())->start(sp);

}

vds::p2p_crypto_tunnel::p2p_crypto_tunnel(const std::shared_ptr<vds::udp_transport::_session> &impl)
: session(impl) {

}

vds::p2p_crypto_tunnel::~p2p_crypto_tunnel() {
}

