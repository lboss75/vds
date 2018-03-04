#ifndef __VDS_P2P_NETWORK_P2P_ROUTE_P_H_
#define __VDS_P2P_NETWORK_P2P_ROUTE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "udp_transport.h"
#include "p2p_node_info.h"
#include "messages/p2p_message_id.h"
#include "node_id_t.h"
#include "messages/dht_pong.h"

namespace vds {
  class _p2p_crypto_tunnel;

  class _p2p_route : public std::enable_shared_from_this<_p2p_route> {
  public:
    struct node {
      node_id_t id_;
      std::shared_ptr<_p2p_crypto_tunnel> proxy_session_;
      uint8_t pinged_;

      node()
        : pinged_(0) {
      }

      node(
        const node_id_t & id,
        const std::shared_ptr<_p2p_crypto_tunnel> proxy_session)
        : id_(id),
        proxy_session_(proxy_session),
        pinged_(0) {
      }

      bool is_good() const {
        return this->pinged_ < 100;
      }

      void reset(const node_id_t & id,
        const std::shared_ptr<_p2p_crypto_tunnel> proxy_session) {
        this->id_ = id;
        this->proxy_session_ = proxy_session;
        this->pinged_ = 0;
      }
    };

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
    timer backgroud_timer_;
    node_id_t current_node_id_;

    struct bucket {
      static constexpr size_t MAX_NODES = 8;

      mutable std::shared_mutex nodes_mutex_;
      std::list<node> nodes_;

      void add_node(
          const service_provider &sp,
          const node_id_t & id,
          const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

      void on_timer(
          const service_provider &sp,
        const _p2p_route * owner);

      bool contains(const node_id_t &node_id) const;
    };

    mutable std::shared_mutex buckets_mutex_;
    std::map<uint8_t, bucket> buckets_;

    void _search_nodes(
      const vds::service_provider &sp,
      const node_id_t & target_id,
      size_t max_count,
      std::list<node> & result_nodes);

    void search_nodes(
        const service_provider &sp,
        const node_id_t &target_id,
        std::map<vds::node_id_t, node> &result_nodes,
        uint8_t index);

    void ping_buckets(const service_provider &sp);

    void update_route_table(const service_provider &sp);

    void find_node(const service_provider &sp, const node_id_t &node_id);

    void for_near(
        const service_provider &sp,
        const node_id_t &target_node_id,
        size_t max_count,
        const std::function<void(const node_id_t & node_id, const std::shared_ptr<_p2p_crypto_tunnel> & session)> & callback);

    void on_timer(
      const service_provider &sp);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_P_H_
