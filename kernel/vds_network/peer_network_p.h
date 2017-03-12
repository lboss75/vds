#ifndef __VDS_NETWORK_PEER_NETWORK_P_H_
#define __VDS_NETWORK_PEER_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "peer_network.h"

namespace vds {
  class _peer_network
  {
  public:
    _peer_network(
      const service_provider & sp,
      peer_network * owner);
    
    void for_each_active_channel(const std::function<void(peer_channel *)> & callback);
    void register_handler(message_handler_base * handler);

  private:
    service_provider sp_;
    peer_network * owner_;
  };
}

#endif//__VDS_NETWORK_PEER_NETWORK_P_H_