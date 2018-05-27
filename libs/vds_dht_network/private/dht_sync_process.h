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
        async_task<> query_unknown_records(const service_provider& sp, database_transaction& t);

        async_task<> do_sync(
          const service_provider & sp,
          database_transaction & t);

        async_task<> apply_message(
          const service_provider & sp,
          database_transaction & t,
          const messages::transaction_log_state & message);

        async_task<> apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::transaction_log_request& message);

        void apply_message(
          const service_provider& sp,
          database_transaction& t,
          const messages::transaction_log_record & message);

        void apply_message(
            const service_provider & sp,
            database_transaction & t,
            const messages::offer_replica & message);

        void apply_message(
            const service_provider & sp,
            database_transaction & t,
            const messages::got_replica & message);

      private:
        void sync_local_channels(
          const service_provider & sp,
          database_transaction & t);

        async_task<> sync_replicas(const service_provider &sp, database_transaction &t);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
