#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_P_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_session.h"
#include "dht_route.h"
#include "chunk.h"
#include "messages/offer_move_replica.h"
#include "messages/replica_request.h"
#include "dht_sync_process.h"
#include "udp_transport.h"

namespace vds {
  namespace dht {
    namespace messages {
      class dht_pong;
      class dht_ping;
      class channel_log_record;
      class channel_log_request;
      class dht_find_node_response;
      class dht_find_node;
    }
  }

  class database_transaction;

  namespace dht {
    namespace network {
      class _client : public std::enable_shared_from_this<_client> {
      public:
        static constexpr uint16_t MIN_HORCRUX = 512;
        static constexpr uint16_t GENERATE_HORCRUX = 1024;

        _client(
            const service_provider & sp,
            const const_data_buffer & node_id);

        void start(const service_provider & sp, uint16_t port);
        void stop(const service_provider & sp);

        void save(
            const service_provider & sp,
            database_transaction & t,
            const const_data_buffer & key,
            const const_data_buffer & value);

        const const_data_buffer &current_node_id() const {
          return this->route_.current_node_id();
        }

        void neighbors(
            const service_provider & sp,
            const const_data_buffer & key,
            std::map<vds::const_data_buffer /*distance*/, std::list<vds::const_data_buffer/*node_id*/>> & result,
            uint16_t max_count) const {
          this->route_.neighbors(sp, key, result, max_count);
        }

        async_task<> apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::channel_log_state & message);

        async_task<> apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::channel_log_request & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::channel_log_record & message);

        async_task<> apply_message(
          const service_provider & sp,
          const messages::dht_find_node & message);

        async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_find_node_response & message);

        async_task<>  apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_ping & message);

        async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_pong & message);

        async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::replica_request & message);

        async_task<> apply_message(
            const service_provider & sp,
            const std::shared_ptr<dht_session> & session,
            const messages::offer_replica & message);

        void apply_message(
            const service_provider & sp,
            database_transaction & t,
            const messages::offer_move_replica & message) {
          this->sync_process_.apply_message(sp, t, message);
        }

        template <typename message_type>
        async_task<> send(
          const service_provider & sp,
          const const_data_buffer & node_id,
          const message_type & message) {
          return this->send(sp, node_id, message_type::message_id, message.serialize());
        }

        void add_session(const service_provider& sp, const std::shared_ptr<dht_session>& session, uint8_t hops);

        async_task<> restore(
            const service_provider &sp,
            const client::chunk_info &block_id,
            const std::shared_ptr<const_data_buffer> & result,
            const std::chrono::steady_clock::time_point & start);

        void get_route_statistics(route_statistic& result);

      private:
        std::shared_ptr<udp_transport> udp_transport_;
        dht_route<std::shared_ptr<dht_session>> route_;
        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
        sync_process sync_process_;

        timer update_timer_;
        std::debug_mutex update_timer_mutex_;
        bool in_update_timer_;

        async_task<> update_route_table(const service_provider& sp);
        async_task<> process_update(
            const service_provider & sp,
            database_transaction & t);

        async_task<> send(
          const service_provider& sp,
          const const_data_buffer& node_id,
          const message_type_t message_id,
          const const_data_buffer& message);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
