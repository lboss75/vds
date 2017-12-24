#ifndef __VDS_P2P_NETWORK_P2P_ROUTE_H_
#define __VDS_P2P_NETWORK_P2P_ROUTE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class p2p_route {
  public:
    p2p_route();
    ~p2p_route();

    async_task<> random_broadcast(
        const vds::service_provider &sp,
        const vds::const_data_buffer &message);

    std::shared_ptr<class _p2p_route> operator -> () const {
      return this->impl_;
    }
  private:
    std::shared_ptr<class _p2p_route> impl_;

  };

}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_H_
