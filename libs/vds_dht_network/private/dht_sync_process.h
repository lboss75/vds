#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_

#include <messages/got_replica.h>
#include "service_provider.h"
#include "database.h"
#include "messages/offer_replica.h"
#include "messages/transaction_log_state.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
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
        async_task<> sync_local_channels(
          const service_provider & sp,
          database_transaction & t);

        async_task<> sync_replicas(const service_provider &sp, database_transaction &t);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
