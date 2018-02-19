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

  static const_data_buffer calc_distance(
    const const_data_buffer & source_node,
    const const_data_buffer & target_node);

  static uint8_t calc_distance_exp(
      const const_data_buffer & source_node,
      const const_data_buffer & target_node);

    void on_timer(
        const service_provider &sp);

  private:
    std::map<guid, std::shared_ptr<_p2p_crypto_tunnel>> sessions_;
    mutable std::shared_mutex sessions_mutex_;

    struct node {
      node_id_t id_;
      std::shared_ptr<_p2p_crypto_tunnel> proxy_session_;
      uint8_t pinged_;

      node(
          const node_id_t & id,
          const std::shared_ptr<_p2p_crypto_tunnel> proxy_session)
          : id_(id),
            proxy_session_(proxy_session),
            pinged_(0){
      }

      bool is_good() const {
        return this->pinged_ < 5;
      }

      void reset(const node_id_t & id,
                 const std::shared_ptr<_p2p_crypto_tunnel> proxy_session){
        this->id_ = id;
        this->proxy_session_ = proxy_session;
        this->pinged_ = 0;
      }
    };

    struct bucket {
      static constexpr size_t MAX_NODES = 8;

      std::shared_mutex nodes_mutex_;
      std::list<node> nodes_;

      void add_node(
          const service_provider &sp,
          const node_id_t & id,
          const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

      void on_timer(
          const service_provider &sp);
    };

    std::shared_mutex buckets_mutex_;
    std::map<uint8_t, bucket> buckets_;

    void add_node(
        const service_provider & sp,
        const node_id_t & id,
        const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

    void search_nodes(
        const vds::service_provider &sp,
        const const_data_buffer & target_id,
        size_t max_count,
        std::vector<node_id_t> & result_nodes);

    void search_nodes(const service_provider &sp, const const_data_buffer &target_id, size_t max_count,
                    std::vector<node_id_t> &result_nodes, uint8_t index);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_ROUTE_P_H_
