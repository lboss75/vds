#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include "database.h"
#include "thread_apartment.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
      class sync_looking_storage_response;
      class sync_looking_storage_request;
      class sync_base_message_request;
      class sync_coronation_response;
      class sync_coronation_request;
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
        void query_unknown_records(const service_provider& sp, database_transaction& t);

        void do_sync(
          const service_provider & sp,
          database_transaction & t);

        async_task<> apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::transaction_log_state & message);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_base_message_request & message);

          void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::transaction_log_request& message);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::transaction_log_record & message);

        void add_sync_entry(
          const service_provider& sp,
          database_transaction& t,
          const const_data_buffer& object_id);

        void apply_message(
          const service_provider& sp,
          const messages::sync_new_election_request & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_new_election_response & message);

        void make_new_follower(
          const service_provider& sp,
          database_transaction& t,
          const messages::sync_coronation_request& message);

        void apply_message(
          const service_provider& sp,
          database_transaction & t,
          const messages::sync_coronation_request & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_coronation_response & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_looking_storage_request & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_looking_storage_response & message);

      private:
        static constexpr std::chrono::system_clock::duration LEADER_BROADCAST_TIMEOUT = std::chrono::minutes(10);
        static constexpr std::chrono::system_clock::duration ELECTION_TIMEOUT = std::chrono::seconds(5);
        static constexpr std::chrono::system_clock::duration CANDITATE_TIMEOUT = std::chrono::seconds(5);
        static constexpr std::chrono::system_clock::duration MEMBER_TIMEOUT = std::chrono::hours(1);

        struct sync_entry {


          sync_entry();

          void make_follower(
            const service_provider& sp,
            const const_data_buffer& object_id,
            const const_data_buffer& source_node,
            uint64_t current_term);

          void make_leader(
            const service_provider& sp,
            const const_data_buffer& object_id);

          void make_canditate(
            const service_provider& sp,
            const const_data_buffer& object_id);


        };

        std::shared_ptr<thread_apartment> sync_object_;
        std::map<const_data_buffer, sync_entry> sync_entries_;

        void sync_entries(
          const service_provider &sp,
          database_transaction & t);


        void sync_local_channels(
          const service_provider & sp,
          database_transaction & t);

        void sync_replicas(const service_provider &sp, database_transaction &t);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
