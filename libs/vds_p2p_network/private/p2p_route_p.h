#ifndef __VDS_P2P_NETWORK_P2P_ROUTE_P_H_
#define __VDS_P2P_NETWORK_P2P_ROUTE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "udp_transport.h"
#include "p2p_node_info.h"
#include "messages/p2p_message_id.h"

namespace vds {
  class _p2p_crypto_tunnel;

  class _p2p_route : public std::enable_shared_from_this<_p2p_route> {
  public:
    async_task<> send_to(
        const service_provider & sp,
        const guid & node_id,
        const const_data_buffer & message);

    bool random_broadcast(
        const service_provider &sp,
        const const_data_buffer &message);

    void add(const service_provider &sp, const guid &partner_id,
                 const std::shared_ptr<_p2p_crypto_tunnel> &session);

    std::set<p2p::p2p_node_info> get_neighbors() const;

    void broadcast(
        const service_provider & sp,
        const const_data_buffer & message) const;

    bool send(
        const service_provider &sp,
        const guid &device_id,
        const const_data_buffer &message);

    void close_session(
        const service_provider &sp,
        const guid &partner,
        const std::shared_ptr<std::exception> & ex);

	void query_replica(
      const service_provider &sp,
      const const_data_buffer & data_hash);

  void query_replica(
    const service_provider &sp,
    const const_data_buffer & data_hash,
    const guid & source_node);

  static size_t calc_distance(
    const const_data_buffer & source_node,
    const const_data_buffer & target_node);

  private:
    std::map<guid, std::shared_ptr<_p2p_crypto_tunnel>> sessions_;
    mutable std::shared_mutex sessions_mutex_;
  };

}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_P_H_
