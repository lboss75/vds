#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>

namespace vds {
  class p2p_crypto_tunnel {
  public:
    p2p_crypto_tunnel();
    ~p2p_crypto_tunnel();


  private:
    std::shared_ptr<class _p2p_crypto_tunnel> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_H_
