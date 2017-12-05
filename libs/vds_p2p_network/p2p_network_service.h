//
// Created by vadim on 30.10.17.
//

#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "async_task.h"

namespace vds {

  class p2p_network_service {
  public:

    vds::async_task<> start(
        const vds::service_provider &sp,
        int port,
        const class certificate &node_cert,
        const class asymmetric_private_key &node_key);

    vds::async_task<> start(
        const vds::service_provider &sp,
        int port,
        const std::string &login,
        const std::string &password);

  private:
    std::shared_ptr<class _p2p_network_service> impl_;
  };

}


#endif //__VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_
