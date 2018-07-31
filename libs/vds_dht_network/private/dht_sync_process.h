#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include "database.h"
#include "sync_message_dbo.h"
#include "messages/dht_message_type.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
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
      class offer_replica;
      class transaction_log_state;
      class transaction_log_record;
      class transaction_log_request;
      class got_replica;
    }
  }
}

namespace vds {
  namespace dht {
    namespace network {
      class dht_session;

      class sync_process {
      public:
        void do_sync(
          const service_provider & sp,
          database_transaction & t);

        void add_local_log(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer & object_id,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer & member_node,
          uint16_t replica,
          const const_data_buffer& leader_node);

        void add_sync_entry(const service_provider &sp, database_transaction &t,
                                    const const_data_buffer &object_id, uint32_t object_size);
        
        static std::set<const_data_buffer> get_members(
          const service_provider& sp,
          database_read_transaction & t,
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

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_new_election_request & message);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_new_election_response & message);

        //void apply_message(
        //  const service_provider& sp,
        //  database_transaction & t,
        //  const messages::sync_coronation_request & message);

        //void apply_message(
        //  const service_provider& sp,
        //  const messages::sync_coronation_response & message);

        void apply_message(
          const service_provider& sp,
          database_transaction & t,
          const messages::sync_looking_storage_request & message);

        void apply_message(
          const service_provider& sp,
          database_transaction & t,
          const messages::sync_looking_storage_response & message);

        void apply_message(
          const service_provider& sp,
          database_transaction & t,
          const messages::sync_snapshot_request & message);

        void apply_message(
          const service_provider& sp,
          database_transaction & t,
          const messages::sync_snapshot_response & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::sync_add_message_request & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::sync_leader_broadcast_request & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::sync_leader_broadcast_response & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::sync_replica_operations_request & message);

        void apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::sync_replica_operations_response & message);

      private:
        static constexpr std::chrono::system_clock::duration FOLLOWER_TIMEOUT = std::chrono::minutes(1);
        static constexpr std::chrono::system_clock::duration LEADER_BROADCAST_TIMEOUT = std::chrono::minutes(10);

        static constexpr std::chrono::system_clock::duration ELECTION_TIMEOUT = std::chrono::seconds(5);
        static constexpr std::chrono::system_clock::duration CANDITATE_TIMEOUT = std::chrono::seconds(5);
        static constexpr std::chrono::system_clock::duration MEMBER_TIMEOUT = std::chrono::hours(1);
        static constexpr std::chrono::system_clock::duration LOCAL_QUEUE_TIMEOUT = std::chrono::seconds(5);

        uint32_t get_quorum(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer & object_id) const;

        void send_leader_broadcast(
          const vds::service_provider& sp,
          vds::database_transaction& t,
          const const_data_buffer & object_id);

        void sync_entries(
          const service_provider &sp,
          database_transaction & t);

        void send_snapshot_request(
          const service_provider& sp,
          const const_data_buffer& object_id,
          const const_data_buffer& leader_node,
          const const_data_buffer& from_node = const_data_buffer());

        const_data_buffer get_leader(
            const service_provider &sp,
            database_transaction &t,
            const const_data_buffer &object_id);

        void apply_record(
          const vds::service_provider &sp,
          vds::database_transaction &t,
          const const_data_buffer &object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t message_index);

        void apply_record(
            const vds::service_provider &sp,
            vds::database_transaction &t,
            const const_data_buffer &object_id,
            orm::sync_message_dbo::message_type_t message_type,
            const const_data_buffer & member_node,
            uint16_t replica,
            uint64_t message_index);

        template<typename message_type>
        void send_to_members(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer & object_id,
          const message_type & message) const {
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
          const messages::sync_base_message_request & message);

        bool apply_base_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_base_message_response & message);

        void send_to_members(
          const service_provider& sp,
          database_read_transaction& t,
          const const_data_buffer & object_id,
          dht::network::message_type_t message_id,
          const const_data_buffer& message_body) const;

        static void send_snapshot(
          const service_provider& sp,
          database_read_transaction & t,
          const const_data_buffer& object_id,
          const std::set<vds::const_data_buffer> & target_nodes);

        void sync_local_queues(
          const service_provider& sp,
          database_transaction& t);

        void make_leader(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
