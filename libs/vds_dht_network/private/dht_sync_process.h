#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include "database.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
      class sync_new_election_response;
      class sync_new_election_request;
      class sync_get_leader_request;
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
          const messages::sync_get_leader_request & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_new_election_request & message);

        void apply_message(
          const service_provider& sp,
          const messages::sync_new_election_response & message);

      private:

        struct sync_entry {
          static constexpr std::chrono::system_clock::duration CONNECTION_TIMEOUT = std::chrono::seconds(1);
          static constexpr std::chrono::system_clock::duration ELECTION_TIMEOUT = std::chrono::seconds(5);
          static constexpr std::chrono::system_clock::duration CANDITATE_TIMEOUT = std::chrono::seconds(5);

          enum class state_t {
            connecting,         
            follower,
            start_election,
            canditate,
            leader
          };

          state_t state_;
          std::chrono::system_clock::time_point last_operation_;

          const_data_buffer leader_;
          uint64_t current_term_;

          const_data_buffer voted_for_;
          uint64_t commit_index_;
          uint64_t last_applied_;

          std::set<const_data_buffer> voted_notes_;

          sync_entry()
          : state_(state_t::connecting),
            last_operation_(std::chrono::system_clock::now()),
            current_term_(0),
            commit_index_(0),
            last_applied_(0) {
          }

          void make_follower(
            const service_provider& sp,
            const const_data_buffer& object_id,
            const const_data_buffer& source_node,
            uint64_t current_term);

          void make_leader(const service_provider& sp);


        };

        std::shared_mutex sync_mutex_;
        std::map<const_data_buffer, sync_entry> sync_entries_;

        void sync_entryes(
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
