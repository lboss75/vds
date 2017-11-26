//
// Created by vadim on 30.10.17.
//

#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "private/p2p_network_p.h"

namespace vds {

  class _p2p_network_service {
  public:
    _p2p_network_service(const vds::service_provider &sp);

    vds::async_task<> start(const vds::service_provider &sp, int port);

    vds::async_task<> start(const vds::service_provider &sp, int port, const std::string &login,
                                const std::string &password);

  private:
    std::shared_ptr<_p2p_network> network_;
  };

}


#endif //__VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_
