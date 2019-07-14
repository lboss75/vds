#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include "database.h"
#include "sync_message_dbo.h"
#include "messages/dht_route_messages.h"
#include "dht_network_client.h"
#include "chunk.h"
#include "imessage_map.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
      class sync_replica_query_operations_request;
      class sync_offer_remove_replica_operation_request;
      class sync_replica_data;
      class sync_replica_request;
      class sync_replica_operations_response;
      class sync_replica_operations_request;
      class sync_leader_broadcast_response;
      class sync_leader_broadcast_request;
      class sync_add_message_request;
      class sync_snapshot_response;
      class sync_snapshot_request;
      class sync_base_message_response;
      class sync_looking_storage_response;
      class sync_looking_storage_request;
      class sync_base_message_request;
      class sync_new_election_response;
      class sync_new_election_request;
      class sync_offer_send_replica_operation_request;
      class transaction_log_state;
      class transaction_log_record;
      class transaction_log_request;
    }
  }
}

namespace vds {
  namespace dht {
    namespace network {
      class dht_session;

      class sync_process {
      public:
        sync_process(const service_provider * sp);

        expected<void> do_sync(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);

        expected<void> add_sync_entry(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          uint32_t object_size);

        expected<const_data_buffer> restore_replica(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const_data_buffer object_id);

        expected<std::list<uint16_t>> prepare_restore_replica(
          database_read_transaction & t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const_data_buffer object_id);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_new_election_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_new_election_response& message,
          const imessage_map::message_info_t& message_info);

        //void apply_message(
        //  
        //  database_transaction & t,
        //  const messages::sync_coronation_request & message);

        //void apply_message(
        //  
        //  const messages::sync_coronation_response & message);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_looking_storage_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_looking_storage_response& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_snapshot_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_snapshot_response& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_add_message_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_leader_broadcast_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_leader_broadcast_response& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_operations_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_operations_response& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_offer_send_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_offer_remove_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_request& message,
          const imessage_map::message_info_t& message_info);

        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_data& message,
          const imessage_map::message_info_t& message_info);
        
        expected<bool> apply_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_replica_query_operations_request & message,
          const imessage_map::message_info_t& message_info);
        
        expected<void> on_new_session(
          database_read_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& partner_id);

      private:
        const service_provider * sp_;

        static std::chrono::system_clock::duration FOLLOWER_TIMEOUT() {
          return std::chrono::minutes(20);
        }

        static std::chrono::system_clock::duration LEADER_BROADCAST_TIMEOUT() {
          return std::chrono::minutes(15);
        }

        static std::chrono::system_clock::duration ELECTION_TIMEOUT() {
          return std::chrono::minutes(10);
        }

        static std::chrono::system_clock::duration CANDITATE_TIMEOUT() {
          return std::chrono::minutes(10);
        }

        static std::chrono::system_clock::duration MEMBER_TIMEOUT() {
          return std::chrono::hours(48);
        }

        static std::chrono::system_clock::duration LOCAL_QUEUE_TIMEOUT() {
          return std::chrono::minutes(5);
        }

        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> distributed_generators_;
        int sync_replicas_timeout_;

        expected<void> add_to_log(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer& member_node,
          uint16_t replica,
          const const_data_buffer& source_node,
          uint64_t source_index);

        expected<void> add_local_log(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer& member_node,
          uint16_t replica,
          const const_data_buffer& leader_node);

        expected<std::set<const_data_buffer>> get_members(
          database_read_transaction& t,
          const const_data_buffer& object_id,
          bool include_removed);

        expected<void> make_new_election(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id);

        expected<void> make_follower(
          database_transaction& t,
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          const const_data_buffer& leader_node);

        expected<uint32_t> get_quorum(
          database_read_transaction& t,
          const const_data_buffer& object_id);

        expected<void> send_leader_broadcast(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id);

        expected<void> sync_entries(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);

        void send_snapshot_request(
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          const const_data_buffer& leader_node,
          const const_data_buffer& from_node = const_data_buffer());

        expected<const_data_buffer> get_leader(          
          database_transaction& t,
          const const_data_buffer& object_id);

        expected<void>  apply_record(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          const const_data_buffer& leader_node_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t message_index,
          uint64_t last_applied);

        expected<void> apply_record(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer& member_node,
          uint16_t replica,
          const const_data_buffer& message_node,
          uint64_t message_index,
          const const_data_buffer& leader_node_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied);

        template <typename message_type>
        expected<void>  send_to_members(
          database_read_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id,
          const message_type& message) const {
          return this->send_to_members(t, final_tasks, object_id, message_type::message_id, message.serialize());
        }

        enum class base_message_type {
          successful,
          not_found,
          from_future,
          from_past,
          other_leader
        };

        expected<base_message_type> apply_base_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_base_message_request& message,
          const imessage_map::message_info_t& message_info,
          const const_data_buffer& leader_node,
          uint64_t last_applied,
          bool allow_snapshot_request = true);

        expected<bool> apply_base_message(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const messages::sync_base_message_response& message,
          const imessage_map::message_info_t& message_info);

        expected<void> send_snapshot(
          database_read_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const_data_buffer object_id,
          std::set<const_data_buffer> target_nodes);

        expected<void> sync_local_queues(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);

        expected<void> make_leader(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& object_id);

        //Sync replicas
        expected<void> sync_replicas(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks);

        class replica_sync {
        public:
         
          expected<void> load(
            const service_provider * sp,
            const database_read_transaction& t);

          expected<void> normalize_density(
            sync_process * owner,
            const service_provider * sp,
            database_transaction& t,
            std::list<std::function<async_task<expected<void>>()>> & final_tasks);
        private:
          struct node_info_t {
            std::set<uint16_t> replicas_;
          };

          struct object_info_t {
            const_data_buffer sync_leader_;
            uint64_t sync_generation_;
            uint64_t sync_current_term_;
            uint64_t sync_commit_index_;
            uint64_t sync_last_applied_;

            std::map<const_data_buffer, node_info_t> nodes_;

            expected<void> restore_chunk(
              const service_provider * sp,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
                               const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                               const const_data_buffer& object_id) const;
            expected<void> generate_missing_replicas(
              const service_provider * sp,
              const database_read_transaction& t,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                                           const const_data_buffer& object_id,
                                           std::set<const_data_buffer> chunk_nodes) const;
            expected<void> restore_replicas(
              const service_provider * sp,
              const database_read_transaction& t,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
              const const_data_buffer& object_id) const;

            /**
             * \brief evenly distribute replicas
             * \param sp 
             * \param replica_nodes 
             * \param object_id 
             */
            expected<void> normalize_density(
              const service_provider * sp,
              const database_read_transaction& t,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                                   const const_data_buffer& object_id) const;

            /**
             * \brief remove replica on many nodes
             * \param sp 
             * \param replica_nodes 
             * \param object_id 
             */
            expected<void> remove_duplicates(
              vds::dht::network::sync_process * owner,
              const service_provider * sp,
              database_transaction & t,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
              const const_data_buffer& object_id) const;

            expected<void> try_to_attach(
              const service_provider * sp,
              std::list<std::function<async_task<expected<void>>()>> & final_tasks,
              const const_data_buffer& object_id) const;

          };

          std::map<const_data_buffer, object_info_t> objects_;

          void register_local_chunk(
            const const_data_buffer& object_id,
            const const_data_buffer& current_node_id);

          void register_replica(
            const const_data_buffer& object_id,
            uint16_t replica,
            const const_data_buffer& node_id);

          void register_sync_leader(
            const const_data_buffer& object_id,
            const const_data_buffer& leader_node_id,
            uint64_t generation,
            uint64_t current_term,
            uint64_t commit_index,
            uint64_t last_applied);

          void register_sync_member(
            const const_data_buffer& object_id,
            const const_data_buffer& member_node);

        };

        expected<void> send_random_replicas(
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          std::map<uint16_t, std::list<std::function<expected<void>()>>> allowed_replicas,
          std::set<uint16_t> send_replicas,
          uint16_t count,
          std::map<size_t, std::set<uint16_t>> replica_frequency);

        static expected<void> send_replica(
          const service_provider * sp,
          const database_read_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer& target_node,
          const const_data_buffer& object_id,
          uint16_t replica,
          const const_data_buffer& leader_node_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied);

        expected<bool> remove_replica(
          database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer & object_id,
          uint16_t replica,
          const const_data_buffer & leader_node);

        expected<std::map<size_t, std::set<uint16_t>>> get_replica_frequency(
          database_transaction& t,
          const const_data_buffer& object_id);

        enum class send_random_replica_goal_t {
          restore_object,
          new_member
        };

        expected<void> send_random_replicas(
          vds::database_transaction& t,
          std::list<std::function<async_task<expected<void>>()>> & final_tasks,
          const const_data_buffer & object_id,
          const const_data_buffer & target_node,
          send_random_replica_goal_t goal,
          const std::set<uint16_t>& exist_replicas);

        expected<void> validate_last_applied(
          vds::database_transaction& t,
          const const_data_buffer & object_id);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
