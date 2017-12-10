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
  class p2p_crypto_tunnel : public udp_transport::session {
  public:
    p2p_crypto_tunnel(const std::shared_ptr<vds::udp_transport::_session> &impl);
    ~p2p_crypto_tunnel();


    void start(const service_provider & sp);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_
