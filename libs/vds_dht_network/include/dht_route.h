#ifndef __VDS_DHT_NETWORK_P2P_ROUTE_H_
#define __VDS_DHT_NETWORK_P2P_ROUTE_H_

#include "const_data_buffer.h"
#include "logger.h"
#include "dht_object_id.h"
#include "legacy.h"
#include <map>
#include "dht_datagram_protocol.h"
#include "iudp_transport.h"
#include "dht_session.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct route_statistic;

  namespace dht {
    class dht_route {
      typedef std::shared_ptr<network::dht_session> session_type;
    public:
      struct node : public std::enable_shared_from_this<node>{
        const_data_buffer node_id_;
        session_type proxy_session_;
        uint8_t pinged_;
        uint8_t hops_;

        node();

        node(
            const const_data_buffer &id,
            const session_type &proxy_session,
          uint8_t hops);
        node(node&& origin);
        node(const node & origin)
          : node_id_(origin.node_id_),
          proxy_session_(origin.proxy_session_),
          hops_(origin.hops_),
          pinged_(origin.pinged_) {
        }

        bool is_good() const;

        void reset(
            const const_data_buffer &id,
            const session_type &proxy_session,
          uint8_t hops);
      };

      dht_route(
        const service_provider * sp,
        const const_data_buffer& this_node_id);

      const const_data_buffer& current_node_id() const;

      bool add_node(
          const const_data_buffer &id,
          const session_type &proxy_session,
          uint8_t hops,
        bool allow_skip);

      vds::async_task<vds::expected<void>> on_timer(std::shared_ptr<network::iudp_transport> transport);

      expected<void>  search_nodes(        
        const const_data_buffer &target_id,
        size_t max_count,
        const std::function<expected<bool>(const node & node)>& filter,
        std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>>& result_nodes) const;

      template<typename callback_type>//std::function<vds::async_task<vds::expected<bool>>(const std::shared_ptr<node> & candidate)>
      vds::async_task<vds::expected<void>> for_near(
        const const_data_buffer &target_node_id,
        size_t max_count,
        const std::function<expected<bool>(const node & node)>& filter,
        callback_type callback) {

        std::map<
            const_data_buffer /*distance*/,
            std::map<const_data_buffer, std::shared_ptr<node>>> result_nodes;
        CHECK_EXPECTED_ASYNC(this->search_nodes(target_node_id, max_count, filter, result_nodes));

        for (auto &presult : result_nodes) {
          for (auto & node : presult.second) {
            auto callback_result = co_await callback(node.second);
            CHECK_EXPECTED_ERROR_ASYNC(callback_result);
            if (!callback_result.value()) {
              co_return expected<void>();
            }
          }
        }

        co_return expected<void>();
      }
      

      void get_neighbors(
        std::list<std::shared_ptr<node>> & result_nodes) const;

      template<typename callback_type>//std::function<vds::async_task<vds::expected<bool>>(const std::shared_ptr<node> & candidate)>
      vds::async_task<vds::expected<void>> for_neighbors(
        callback_type callback) {

        std::list<std::shared_ptr<node>> result_nodes;
        this->get_neighbors(result_nodes);

        for (auto & node : result_nodes) {
          auto callback_result = co_await callback(node);
          CHECK_EXPECTED_ERROR_ASYNC(callback_result);
          if (!callback_result.value()) {
            co_return expected<void>();
          }
        }

        co_return expected<void>();
      }

      void mark_pinged(const const_data_buffer& target_node, const network_address& address);

      void get_statistics(route_statistic& result);
      void remove_session(
        
        const session_type& session);

    private:
      const service_provider * sp_;
      const_data_buffer current_node_id_;

      struct bucket : public std::enable_shared_from_this<bucket> {
        static constexpr size_t MAX_NODES = 8;

        mutable std::shared_mutex nodes_mutex_;
        std::list<std::shared_ptr<node>> nodes_;

        bool add_node(
            
            const const_data_buffer &id,
            const session_type &proxy_session,
            uint8_t hops,
          bool allow_skip);

        void remove_session(
          const session_type& proxy_session);

        vds::async_task<vds::expected<void>> on_timer(
            const service_provider * sp,
            const dht_route* owner,
            const std::shared_ptr<network::iudp_transport> & transport);

        bool contains(const const_data_buffer& node_id) const;

        void mark_pinged(const const_data_buffer& target_node, const network_address& address);

        void get_statistics(route_statistic& result);

        void get_neighbors(std::list<std::shared_ptr<node>>& result_nodes) const;
      };

      mutable std::shared_mutex buckets_mutex_;
      std::map<size_t, std::shared_ptr<bucket>> buckets_;

      expected<void> _search_nodes(        
        const const_data_buffer &target_id,
        size_t max_count,
        const std::function<expected<bool>(const node & node)>& filter,
        std::map<const_data_buffer /*distance*/, std::map<const_data_buffer, std::shared_ptr<node>>>& result_nodes) const;

      expected<size_t> looking_nodes(
          const const_data_buffer &target_id,
          const lambda_holder_t<expected<bool>, const node &> & filter,
          std::map<const_data_buffer, std::map<const_data_buffer, std::shared_ptr<node>>> &result_nodes,
        size_t index) const;

      vds::async_task<vds::expected<void>> ping_buckets(std::shared_ptr<network::iudp_transport> transport);
    };
  }
}



#endif //__VDS_DHT_NETWORK_P2P_ROUTE_H_
