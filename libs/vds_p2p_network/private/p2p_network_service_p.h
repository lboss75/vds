//
// Created by vadim on 30.10.17.
//

#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdafx.h>
#include "service_provider.h"
#include "private/p2p_network_p.h"
#include "leak_detect.h"

namespace vds {
  class _p2p_crypto_tunnel;

  class _p2p_network_service : public std::enable_shared_from_this<_p2p_network_service> {
  public:
    _p2p_network_service(const vds::service_provider &sp);

    async_task<> start(
        const vds::service_provider &sp,
        int port,
        const std::list<certificate> &certificate_chain,
        const asymmetric_private_key &node_key);

    async_task<> prepare_to_stop(const vds::service_provider &sp);
    void stop(const service_provider & sp);

    void connect(const service_provider & sp, const std::string & address);
  private:
    udp_transport transport_;
    timer backgroud_timer_;

    std::shared_mutex sessions_mutex_;
    std::list<_p2p_crypto_tunnel> sessions_;

    void start_network(const service_provider &sp, int port);

    bool do_backgroud_tasks(const service_provider &sp);
  };

}


#endif //__VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_P_H_
