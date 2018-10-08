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
#include "sync_process.h"
#include "udp_transport.h"
#include "imessage_map.h"

class mock_server;

namespace vds {
  namespace dht {
    namespace messages {
      class sync_replica_operations_response;
      class sync_replica_operations_request;
      class sync_leader_broadcast_response;
      class sync_leader_broadcast_request;
      class sync_add_message_request;
      class sync_replica_data;
      class sync_replica_request;
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
        _client(
          const service_provider * sp,
          const std::shared_ptr<iudp_transport> & udp_transport,
          const std::shared_ptr<certificate> & node_cert,
          const std::shared_ptr<asymmetric_private_key> & node_key);

        void start();
        void stop();
        void get_neighbors(
          
          std::list<std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>& result);
        
        void on_new_session(
          
          database_read_transaction& t,
          const const_data_buffer& partner_id);

        static filename save_data(
          const service_provider * sp,
          database_transaction& t,
          const const_data_buffer& data_hash,
          const const_data_buffer& data);

        std::vector<const_data_buffer> save(
          
          database_transaction& t,
          const const_data_buffer& value);

        const const_data_buffer& current_node_id() const {
          return this->route_.current_node_id();
        }

        void neighbors(
          
          const const_data_buffer& key,
          std::map<const_data_buffer /*distance*/, std::list<const_data_buffer/*node_id*/>>& result,
          uint16_t max_count) const {
          this->route_.neighbors(key, result, max_count);
        }

        void apply_message(
          
          const messages::dht_find_node& message,
          const imessage_map::message_info_t& message_info);

        std::future<void> apply_message(
          
          const messages::dht_find_node_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          const messages::dht_ping& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          const messages::dht_pong& message,
          const imessage_map::message_info_t& message_info);

        //Sync messages
        void apply_message(
          
          database_transaction& t,
          const messages::sync_new_election_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_new_election_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_add_message_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_leader_broadcast_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_leader_broadcast_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_replica_operations_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_replica_operations_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_looking_storage_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_looking_storage_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_snapshot_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_snapshot_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_offer_send_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_offer_remove_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_replica_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_replica_data& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          
          database_transaction& t,
          const messages::sync_replica_query_operations_request & message,
          const imessage_map::message_info_t& message_info);

        //
        template <typename message_type>
        std::future<void> send(
          
          const const_data_buffer& node_id,
          const message_type& message) {
          co_await this->send(node_id, message_type::message_id, message.serialize());
        }

        std::future<void> send(
          
          const const_data_buffer& node_id,
          message_type_t message_id,
          const const_data_buffer& message);

        void find_nodes(
            
            const const_data_buffer& node_id,
            size_t radius);

        template <typename message_type>
        void send_near(
          
          const const_data_buffer& node_id,
          size_t radius,
          const message_type& message) {
          this->send_near(node_id, radius, message_type::message_id, message.serialize());
        }

        template <typename message_type>
        void send_near(
          
          const const_data_buffer& node_id,
          size_t radius,
          const message_type& message,
          const std::function<bool(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter) {
          this->send_near(node_id, radius, message_type::message_id, message.serialize(), filter);
        }

        template <typename message_type>
        std::future<void> send_neighbors(
          
          const message_type& message) {
          co_await this->send_neighbors(message_type::message_id, message.serialize());
        }

        void add_session( const std::shared_ptr<dht_session>& session, uint8_t hops);

        std::future<void> restore(
          
          const std::vector<const_data_buffer>& object_ids,
          const std::shared_ptr<const_data_buffer>& result,
          const std::chrono::steady_clock::time_point& start);

        std::future<uint8_t> restore_async(
          
          const std::vector<const_data_buffer>& object_ids,
          const std::shared_ptr<const_data_buffer>& result);

        void get_route_statistics(route_statistic& result);
        void get_session_statistics(session_statistic& session_statistic);

        void add_route(
          
          const const_data_buffer& source_node,
          uint16_t hops,
          const std::shared_ptr<dht_session>& session);

        void remove_session(
          
          const std::shared_ptr<dht_session>& session);

        void add_sync_entry(
            database_transaction& t,
            const const_data_buffer& object_id,
            uint32_t object_size) {
          this->sync_process_.add_sync_entry(t, object_id, object_size);
        }
      private:
        friend class sync_process;
        friend class dht_session;
        friend class mock_server;

        const service_provider * sp_;
        std::shared_ptr<iudp_transport> udp_transport_;
        dht_route<std::shared_ptr<dht_session>> route_;
        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
        sync_process sync_process_;

        timer update_timer_;
        std::debug_mutex update_timer_mutex_;
        bool in_update_timer_ = false;

        uint32_t update_route_table_counter_;

        std::future<void> update_route_table();
        std::future<void> process_update(
          
          database_transaction& t);


        std::future<void> send_near(
          
          const const_data_buffer& node_id,
          size_t radius,
          message_type_t message_id,
          const const_data_buffer& message);

        std::future<void> send_near(
          
          const const_data_buffer& node_id,
          size_t radius,
          message_type_t message_id,
          const const_data_buffer& message,
          const std::function<bool(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter);

        std::future<void> proxy_message(
            
            const const_data_buffer &node_id,
            message_type_t message_id,
            const const_data_buffer &message,
            const const_data_buffer &source_node,
            uint16_t hops);

        std::future<void> send_neighbors(
          
          message_type_t message_id,
          const const_data_buffer& message);

        static const_data_buffer replica_id(
          const std::string& key,
          uint16_t replica);

        std::future<void> update_wellknown_connection(
          
          database_transaction& t);

        static const_data_buffer read_data(
          const const_data_buffer& data_hash,
          const filename& data_path);

        static void delete_data(
          const const_data_buffer& replica_hash,
          const filename& filename);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
