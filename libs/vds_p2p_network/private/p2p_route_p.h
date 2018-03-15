#ifndef __VDS_P2P_NETWORK_P2P_ROUTE_P_H_
#define __VDS_P2P_NETWORK_P2P_ROUTE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "udp_transport.h"
#include "p2p_node_info.h"
#include "messages/p2p_message_id.h"
#include "messages/dht_pong.h"

namespace vds {
  class _p2p_crypto_tunnel;

  class _p2p_route : public std::enable_shared_from_this<_p2p_route> {
  public:
    _p2p_route();

    void start(const service_provider &sp);
    void stop(const service_provider &sp);

    void add_node(
        const service_provider & sp,
        const node_id_t & id,
        const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

    bool send(
        const service_provider &sp,
        const node_id_t & node_id,
        const const_data_buffer &message,
        bool allow_skip);

    void search_nodes(
        const vds::service_provider &sp,
        const node_id_t & target_id,
        size_t max_count,
        std::list<node> & result_nodes);

    void close_session(
        const vds::service_provider &sp,
        const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

    const vds::node_id_t & current_node_id() const {
      return this->current_node_id_;
    }

    void
    query_replica(
        const service_provider &sp,
        const const_data_buffer & data_id,
        const std::set<uint16_t> &exist_replicas,
        uint16_t distance);
    void get_statistic(const service_provider& sp, class p2p_network_statistic& result);
    void apply(const service_provider& sp, const std::shared_ptr<_p2p_crypto_tunnel>& session, const p2p_messages::dht_pong& message);

  private:
    dht::dht_route<_p2p_crypto_tunnel> route_;
    timer backgroud_timer_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_P_H_
