#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class ip2p_network_storage;
  class _p2p_network;

  class p2p_network {
  public:
    p2p_network(const std::shared_ptr<ip2p_network_storage> & storage);

    void login(const std::string & login, const std::string & password);


  private:
    std::shared_ptr<_p2p_network> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_H_
