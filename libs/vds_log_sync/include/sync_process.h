#ifndef __VDS_LOG_SYNC_SYNC_PROCESS_H_
#define __VDS_LOG_SYNC_SYNC_PROCESS_H_

#include "database.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace messages {
      class transaction_log_record;
      class transaction_log_request;
      class transaction_log_state;
    }
  }
}

namespace vds {
  namespace  transaction_log {
    class sync_process {
    public:
      void do_sync(
        const service_provider & sp,
        database_transaction & t);

      async_task<> apply_message(
        const service_provider & sp,
        database_transaction & t,
        const dht::messages::transaction_log_state & message);

      void apply_message(
        const service_provider& sp,
        database_transaction& t,
        const dht::messages::transaction_log_request& message);

      void apply_message(
        const service_provider& sp,
        database_transaction& t,
        const dht::messages::transaction_log_record & message);

    private:
      void query_unknown_records(const service_provider& sp, database_transaction& t);


      void sync_local_channels(
        const service_provider & sp,
        database_transaction & t);

      void sync_replicas(
        const service_provider &sp,
        database_transaction &t);
    };
  }
}

#endif //__VDS_LOG_SYNC_SYNC_PROCESS_H_
