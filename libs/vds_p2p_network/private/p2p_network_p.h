#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>

namespace vds {
  class ip2p_network_storage;

  class _p2p_network : public std::enable_shared_from_this<_p2p_network> {
  public:
    _p2p_network(const std::shared_ptr<ip2p_network_storage> & storage);
    ~_p2p_network();

  private:
    std::shared_ptr<ip2p_network_storage> storage_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_P_H_
