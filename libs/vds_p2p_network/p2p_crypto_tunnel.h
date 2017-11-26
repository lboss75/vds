#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include <const_data_buffer.h>
#include "udp_transport.h"

namespace vds {
  class p2p_crypto_tunnel {
  public:
    p2p_crypto_tunnel();
    ~p2p_crypto_tunnel();


    void start(
        const vds::udp_transport::session &session,
        const std::string &login,
        const std::string &password);

    void start(
        const vds::udp_transport::session &session,
        const class certificate &node_cert,
        const class asymmetric_private_key &node_key);

    async_task<> handle_incoming_message(
        const service_provider & sp,
        const vds::const_data_buffer &message);

  private:
    std::shared_ptr<class _p2p_crypto_tunnel> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_
