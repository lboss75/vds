#ifndef __VDS_NETWORK_PEER_NETWORK_H_
#define __VDS_NETWORK_PEER_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class peer_network
  {
  public:
    peer_network();
    ~peer_network();

    void open_connection(const std::string & uri);

  private:
    class _peer_network * impl_;

  };
}

#endif//__VDS_NETWORK_PEER_NETWORK_H_