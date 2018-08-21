#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include "database.h"
#include "sync_message_dbo.h"
#include "messages/dht_message_type.h"
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
        sync_process();

        void do_sync(
          const service_provider& sp,
          database_transaction& t);

        void add_sync_entry(const service_provider& sp, database_transaction& t,
                            const const_data_buffer& object_id, uint32_t object_size);

        const_data_buffer restore_replica(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_new_election_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_new_election_response& message,
          const imessage_map::message_info_t& message_info);

        //void apply_message(
        //  const service_provider& sp,
        //  database_transaction & t,
        //  const messages::sync_coronation_request & message);

        //void apply_message(
        //  const service_provider& sp,
        //  const messages::sync_coronation_response & message);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_looking_storage_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_looking_storage_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_snapshot_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_snapshot_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_add_message_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_leader_broadcast_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_leader_broadcast_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_replica_operations_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_replica_operations_response& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_offer_send_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_offer_remove_replica_operation_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_replica_request& message,
          const imessage_map::message_info_t& message_info);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_replica_data& message,
          const imessage_map::message_info_t& message_info);

      private:

        static std::chrono::system_clock::duration FOLLOWER_TIMEOUT() {
          return std::chrono::seconds(10);
        }

        static std::chrono::system_clock::duration LEADER_BROADCAST_TIMEOUT() {
          return std::chrono::minutes(10);
        }

        static std::chrono::system_clock::duration ELECTION_TIMEOUT() {
          return std::chrono::minutes(1);
        }

        static std::chrono::system_clock::duration CANDITATE_TIMEOUT() {
          return std::chrono::seconds(5);
        }

        static std::chrono::system_clock::duration MEMBER_TIMEOUT() {
          return std::chrono::hours(1);
        }

        static std::chrono::system_clock::duration LOCAL_QUEUE_TIMEOUT() {
          return std::chrono::seconds(5);
        }

        std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> distributed_generators_;
        int sync_replicas_timeout_;

        static void add_to_log(const service_provider& sp, database_transaction& t,
                        const const_data_buffer& object_id,
                        orm::sync_message_dbo::message_type_t message_type,
                        const const_data_buffer& member_node,
                        uint16_t replica,
                        const const_data_buffer& source_node,
                        uint64_t source_index);

        static void add_local_log(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer& member_node,
          uint16_t replica,
          const const_data_buffer& leader_node);

        static std::set<const_data_buffer> get_members(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer& object_id);

        void make_new_election(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id) const;

        void make_follower(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          const const_data_buffer& leader_node);

        static uint32_t get_quorum(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer& object_id);

        void send_leader_broadcast(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);

        void sync_entries(
          const service_provider& sp,
          database_transaction& t);

        void send_snapshot_request(
          const service_provider& sp,
          const const_data_buffer& object_id,
          const const_data_buffer& leader_node,
          const const_data_buffer& from_node = const_data_buffer());

        const_data_buffer get_leader(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);

        static void apply_record(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t message_index);

        static void apply_record(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer& member_node,
          uint16_t replica,
          const const_data_buffer& message_node,
          uint64_t message_index);

        template <typename message_type>
        void send_to_members(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer& object_id,
          const message_type& message) const {
          this->send_to_members(sp, t, object_id, message_type::message_id, message.serialize());
        }

        enum class base_message_type {
          successful,
          not_found,
          from_future,
          from_past,
          other_leader
        };

        base_message_type apply_base_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_base_message_request& message,
          const imessage_map::message_info_t& message_info,
          const const_data_buffer& leader_node);

        bool apply_base_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_base_message_response& message,
          const imessage_map::message_info_t& message_info);

        static void send_snapshot(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer& object_id,
          const std::set<const_data_buffer>& target_nodes);

        void sync_local_queues(
          const service_provider& sp,
          database_transaction& t);

        void make_leader(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);

        //Sync replicas
        void sync_replicas(
          const service_provider& sp,
          database_transaction& t);

        class replica_sync {
        public:
          void load(
            const service_provider& sp,
            database_transaction& t);

          void normalize_density(
            const service_provider& sp,
            database_transaction& t);
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

            void restore_chunk(const service_provider& sp,
                               const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                               const const_data_buffer& object_id) const;
            void generate_missing_replicas(const service_provider& sp,
                                           const database_read_transaction& t,
                                           const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                                           const const_data_buffer& object_id,
                                           std::set<const_data_buffer> chunk_nodes) const;
            void restore_replicas(
              const service_provider& sp,
              const database_read_transaction& t,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
              const const_data_buffer& object_id) const;

            /**
             * \brief evenly distribute replicas
             * \param sp 
             * \param replica_nodes 
             * \param object_id 
             */
            void normalize_density(const service_provider& sp,
                                   const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
                                   const const_data_buffer& object_id) const;

            /**
             * \brief remove replica on many nodes
             * \param sp 
             * \param replica_nodes 
             * \param object_id 
             */
            void remove_duplicates(
              const service_provider& sp,
              database_transaction & t,
              const std::map<uint16_t, std::set<const_data_buffer>>& replica_nodes,
              const const_data_buffer& object_id) const;

            void try_to_attach(
              const service_provider& sp,
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

        static void send_random_replicas(
          std::map<uint16_t, std::list<std::function<void()>>>& allowed_replicas,
          std::set<uint16_t>& send_replicas);

        static void send_replica(
          const service_provider& sp,
          const database_read_transaction& t,
          const const_data_buffer& target_node,
          const const_data_buffer& object_id,
          uint16_t replica,
          const const_data_buffer& leader_node_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied);

        static void remove_replica(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer & object_id,
          uint16_t replica,
          const const_data_buffer & leader_node);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
