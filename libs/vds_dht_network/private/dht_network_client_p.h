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
#include "dht_sync_process.h"
#include "udp_transport.h"

namespace vds {
  namespace dht {
    namespace messages {
      class replica_data;
      class object_request;
      class dht_pong;
      class dht_ping;
      class transaction_log_record;
      class transaction_log_request;
      class dht_find_node_response;
      class dht_find_node;
    }
  }

  class database_transaction;

  namespace dht {
    namespace network {
      class _client : public std::enable_shared_from_this<_client> {
      public:
        static constexpr uint16_t MIN_HORCRUX = 4;
        static constexpr uint16_t GENERATE_HORCRUX = 8;

        static constexpr uint16_t MIN_DISTRIBUTED_PIECES = 4;
        static constexpr uint16_t GENERATE_DISTRIBUTED_PIECES = 8;

        _client(
            const service_provider & sp,
            const const_data_buffer & node_id);

        void start(const service_provider & sp, uint16_t port);
        void stop(const service_provider & sp);

        filename save_data(
          const service_provider& sp,
          database_transaction & t,
          const const_data_buffer & data_hash,
          const const_data_buffer& data);

        std::vector<const_data_buffer> save(
            const service_provider & sp,
            database_transaction & t,
            const const_data_buffer & value);

        void save(
          const service_provider & sp,
          database_transaction & t,
          const std::string & name,
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
          const messages::transaction_log_state & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::transaction_log_request & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::transaction_log_record & message);

        void apply_message(
          const service_provider & sp,
          const messages::dht_find_node & message);

        async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_find_node_response & message);

        void  apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_ping & message);

        void apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::dht_pong & message);

        async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::object_request & message);

        vds::async_task<> apply_message(
          const service_provider & sp,
          const std::shared_ptr<dht_session> & session,
          const messages::replica_data & message);
        

        async_task<> apply_message(
            const service_provider & sp,
            const std::shared_ptr<dht_session> & session,
            const messages::offer_replica & message);

        async_task<> apply_message(
          const service_provider& sp,
          const std::shared_ptr<dht_session>& session,
          const messages::got_replica& message);

        void apply_message(
          const service_provider & sp,
          const messages::sync_new_election_request & message);

        void apply_message(
          const service_provider & sp,
          const messages::sync_new_election_response & message);

        void apply_message(
          const service_provider & sp,
          const messages::sync_coronation_request & message);

        void apply_message(
          const service_provider & sp,
          const messages::sync_coronation_response & message);

        template <typename message_type>
        void send(
          const service_provider & sp,
          const const_data_buffer & node_id,
          const message_type & message) {
          this->send(sp, node_id, message_type::message_id, message.serialize());
        }

        template <typename message_type>
        void send_near(
          const service_provider & sp,
          const const_data_buffer & node_id,
          size_t radius,
          const message_type & message) {
          this->send_near(sp, node_id, radius, message_type::message_id, message.serialize());
        }

        template <typename message_type>
        void send_closer(
          const service_provider & sp,
          const const_data_buffer & node_id,
          size_t radius,
          const message_type & message) {
          this->send_closer(sp, node_id, radius, message_type::message_id, message.serialize());
        }

        template <typename message_type>
        void send_neighbors(
          const service_provider& sp,
          const message_type & message) {
          this->send_neighbors(sp, message_type::message_id, message.serialize());
        }

        void add_session(const service_provider& sp, const std::shared_ptr<dht_session>& session, uint8_t hops);

        async_task<> restore(
            const service_provider &sp,
            const std::vector<const_data_buffer> & object_ids,
            const std::shared_ptr<const_data_buffer> & result,
            const std::chrono::steady_clock::time_point & start);

        async_task<> restore(
            const service_provider &sp,
            const std::string & name,
            const std::shared_ptr<const_data_buffer> &result,
            const std::chrono::steady_clock::time_point &start);

        async_task<uint8_t> restore_async(
          const service_provider &sp,
          const std::vector<const_data_buffer> & object_ids,
          const std::shared_ptr<const_data_buffer> & result);

        async_task<uint8_t> restore_async(
          const service_provider &sp,
          const std::string & name,
          const std::shared_ptr<const_data_buffer> &result);

        void get_route_statistics(route_statistic& result);
        void get_session_statistics(session_statistic& session_statistic);

      private:
        std::shared_ptr<udp_transport> udp_transport_;
        dht_route<std::shared_ptr<dht_session>> route_;
        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> distributed_generators_;
        sync_process sync_process_;

        timer update_timer_;
        std::debug_mutex update_timer_mutex_;
        bool in_update_timer_ = false;

        uint32_t update_route_table_counter_;

        async_task<> update_route_table(const service_provider& sp);
        async_task<> process_update(
            const service_provider & sp,
            database_transaction & t);

        void send(
          const service_provider& sp,
          const const_data_buffer& node_id,
          const message_type_t message_id,
          const const_data_buffer& message);

        void send_near(
          const service_provider& sp,
          const const_data_buffer& node_id,
          size_t radius,
          const message_type_t message_id,
          const const_data_buffer& message);

        void send_closer(
          const service_provider& sp,
          const const_data_buffer& node_id,
          size_t radius,
          const message_type_t message_id,
          const const_data_buffer& message);

        void send_neighbors(
          const service_provider& sp,
          const message_type_t message_id,
          const const_data_buffer& message);

        static const_data_buffer replica_id(
          const std::string & key,
          uint16_t replica);

        async_task<> update_wellknown_connection(
            const service_provider &sp,
            database_transaction &t);

        static const_data_buffer read_data(
            const std::string &data_hash,
            const filename &data_path);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
