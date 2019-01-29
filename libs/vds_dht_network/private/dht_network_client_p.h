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
      class client_save_stream;

      class _client : public std::enable_shared_from_this<_client> {
      public:

        _client(
          const service_provider * sp,
          const std::shared_ptr<iudp_transport> & udp_transport,
          const const_data_buffer & this_node_id,
          const std::shared_ptr<asymmetric_private_key> & node_key);

        static expected<std::shared_ptr<_client>> create(
          const service_provider * sp,
          const std::shared_ptr<iudp_transport> & udp_transport,
          const std::shared_ptr<certificate> & node_cert,
          const std::shared_ptr<asymmetric_private_key> & node_key);

        expected<void> start();
        void stop();
        void get_neighbors(
          
          std::list<std::shared_ptr<dht_route<std::shared_ptr<dht_session>>::node>>& result);
        
        expected<void> on_new_session(
          database_read_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& partner_id);

        static expected<filename> save_data(
          const service_provider * sp,
          database_transaction& t,
          const const_data_buffer& data_hash,
          const const_data_buffer& data);

        static expected<filename> save_data(
          const service_provider * sp,
          database_transaction& t,
          const const_data_buffer& data_hash,
          const filename & original_file);

        expected<std::vector<vds::const_data_buffer>> save(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& value);

        expected<std::shared_ptr<client_save_stream>> create_save_stream();

        const const_data_buffer& current_node_id() const {
          return this->route_.current_node_id();
        }

        void neighbors(
          
          const const_data_buffer& key,
          std::map<const_data_buffer /*distance*/, std::list<const_data_buffer/*node_id*/>>& result,
          uint16_t max_count) const {
          this->route_.neighbors(key, result, max_count);
        }

        async_task<expected<void>> apply_message(
          
          const messages::dht_find_node& message,
          const imessage_map::message_info_t& message_info);

        async_task<expected<void>> apply_message(
          
          const messages::dht_find_node_response& message,
          const imessage_map::message_info_t& message_info);

        async_task<expected<void>> apply_message(
          
          const messages::dht_ping& message,
          const imessage_map::message_info_t& message_info);

        async_task<expected<void>> apply_message(
          
          const messages::dht_pong& message,
          const imessage_map::message_info_t& message_info);

        //Sync messages
        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_new_election_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_new_election_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(          
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_add_message_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_leader_broadcast_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_leader_broadcast_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_operations_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_operations_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_looking_storage_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_looking_storage_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_snapshot_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_snapshot_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_offer_send_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_offer_remove_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_request& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_data& message,
          const imessage_map::message_info_t& message_info);

        expected<void> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_query_operations_request & message,
          const imessage_map::message_info_t& message_info);

        //
        template <typename message_type>
        async_task<expected<void>> send(
          const const_data_buffer& node_id,
          expected<message_type> && message) {
          CHECK_EXPECTED_ERROR(message);
          return this->send(node_id, message_type::message_id, message_serialize(message.value()));
        }

        async_task<expected<void>> send(
          const const_data_buffer& node_id,
          message_type_t message_id,
          expected<const_data_buffer> && message);

        async_task<expected<void>> find_nodes(
            
            const const_data_buffer& node_id,
            size_t radius);

        template <typename message_type>
        async_task<expected<void>> send_near(
          
          const const_data_buffer& node_id,
          size_t radius,
          expected<message_type> && message) {
          CHECK_EXPECTED_ERROR(message);
          GET_EXPECTED(message_data, message_serialize(message.value()));
          return this->send_near(node_id, radius, message_type::message_id, message_data);
        }

        template <typename message_type>
        async_task<expected<void>> send_near(
          const const_data_buffer& node_id,
          size_t radius,
          expected<message_type> && message,
          const std::function<expected<bool>(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter) {
          GET_EXPECTED(message_data, message_serialize(message.value()));
          return this->send_near(node_id, radius, message_type::message_id, message_data, filter);
        }

        template <typename message_type>
        vds::async_task<vds::expected<void>> send_neighbors(
          expected<message_type> && message) {

          CHECK_EXPECTED_ERROR(message);

          return this->send_neighbors(message_type::message_id, message_serialize(message.value()));
        }

        async_task<expected<void>> add_session(
          const std::shared_ptr<dht_session>& session,
          uint8_t hops);

        async_task<expected<void>> restore(          
          const std::vector<const_data_buffer>& object_ids,
          const std::shared_ptr<const_data_buffer>& result,
          const std::chrono::steady_clock::time_point& start);

        expected<client::block_info_t> prepare_restore(
          database_read_transaction & t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const std::vector<const_data_buffer>& object_ids);

        async_task<vds::expected<uint8_t>> restore_async(
          
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

        expected<void> add_sync_entry(
            database_transaction& t,
            std::list<std::function<async_task<expected<void>>()>> & final_tasks,
            const const_data_buffer& object_id,
            uint32_t object_size) {
          return this->sync_process_.add_sync_entry(t, final_tasks, object_id, object_size);
        }

        void update_wellknown_connection_enabled(bool value) {
          this->update_wellknown_connection_enabled_ = value;
        }

      private:
        friend class sync_process;
        friend class dht_session;
        friend class mock_server;
        friend class client_save_stream;

        const service_provider * sp_;
        std::shared_ptr<iudp_transport> udp_transport_;
        dht_route<std::shared_ptr<dht_session>> route_;
        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
        sync_process sync_process_;

        timer update_timer_;
        uint32_t update_route_table_counter_;
        bool update_wellknown_connection_enabled_;
        vds::async_task<vds::expected<void>> update_route_table();
        vds::expected<void> process_update(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);


        async_task<vds::expected<void>> send_near(
          const const_data_buffer& node_id,
          size_t radius,
          message_type_t message_id,
          const const_data_buffer& message);

        async_task<vds::expected<void>> send_near(
          const const_data_buffer& node_id,
          size_t radius,
          message_type_t message_id,
          const const_data_buffer& message,
          const std::function<expected<bool>(const dht_route<std::shared_ptr<dht_session>>::node& node)>& filter);

        vds::async_task<vds::expected<void>> proxy_message(
            
            const const_data_buffer &node_id,
            message_type_t message_id,
            const const_data_buffer &message,
            const const_data_buffer &source_node,
            uint16_t hops);

        vds::async_task<vds::expected<void>> send_neighbors(
          message_type_t message_id,
          expected<const_data_buffer> && message);

        static expected<const_data_buffer> replica_id(
          const std::string& key,
          uint16_t replica);

        expected<void> update_wellknown_connection(          
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);

        static expected<const_data_buffer> read_data(
          const const_data_buffer& data_hash,
          const filename& data_path);

        static expected<void> delete_data(
          const const_data_buffer& replica_hash,
          const filename& filename);

        expected<void> restore_async(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const std::vector<const_data_buffer>& object_ids,
          const std::shared_ptr<const_data_buffer>& result,
          const std::shared_ptr<uint8_t> & result_progress);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
