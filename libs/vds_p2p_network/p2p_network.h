#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <set>
#include "udp_transport.h"
#include "p2p_node_info.h"

namespace vds {
  class p2p_network {
  public:
    p2p_network();
    ~p2p_network();

    vds::async_task<> start_network(const vds::service_provider &sp);

    void random_broadcast(
        const service_provider &sp,
        const const_data_buffer &message);

    void broadcast(
        const service_provider & sp,
        const const_data_buffer & message);

    void send(
        const service_provider & sp,
        const guid & device_id,
        const const_data_buffer & message);

    std::shared_ptr<class _p2p_network> operator -> () const {
      return this->impl_;
    }

    std::set<p2p::p2p_node_info> get_neighbors() const;
    std::set<guid> active_nodes() const;

    async_task<> prepare_to_stop(const vds::service_provider &sp);
    void stop(const vds::service_provider &sp);

    operator bool () const {
      return nullptr != this->impl_.get();
    }

    void close_session(
        const service_provider &sp,
        const guid &partner,
        const std::shared_ptr<std::exception> & ex);

	void query_replica(
		const service_provider &sp, 
		const const_data_buffer & data_hash);

  private:
    std::shared_ptr<class _p2p_network> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_H_
