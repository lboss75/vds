#ifndef __VDS_P2P_NETWORK_P2P_ROUTE_P_H_
#define __VDS_P2P_NETWORK_P2P_ROUTE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "udp_transport.h
namespace vds {

  class _p2p_route {
  public:

    async_task<> send_to(
        const service_provider & sp,
        const guid & node_id,
        const const_data_buffer & message);

  private:
    class session {
    public:

    private:
      udp_transport::session target_;
      bool is_busy_;
      guid node_id_;
    };

    std::map<guid, std::shared_ptr<udp_transport::session>> sessions_;
    std::shared_mutex sessions_mutex_;
  };

}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_P_H_
