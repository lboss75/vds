#ifndef __VDS_LOG_SYNC_SYNC_PROCESS_H_
#define __VDS_LOG_SYNC_SYNC_PROCESS_H_

#include "database.h"
#include "imessage_map.h"

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

      vds::async_task<void> apply_message(
        const service_provider & sp,
        database_transaction & t,
        const dht::messages::transaction_log_state & message,
        const dht::network::imessage_map::message_info_t & message_info);

      void apply_message(
        const service_provider& sp,
        database_transaction& t,
        const dht::messages::transaction_log_request& message,
        const dht::network::imessage_map::message_info_t & message_info);

      void apply_message(
        const service_provider& sp,
        database_transaction& t,
        const dht::messages::transaction_log_record & message,
        const dht::network::imessage_map::message_info_t & message_info);

      void on_new_session(
        const service_provider& sp,
        database_read_transaction & t,
        const const_data_buffer& partner_id);

    private:
      void query_unknown_records(const service_provider& sp, database_transaction& t);


      void sync_local_channels(
        const service_provider & sp,
        database_read_transaction & t,
        const const_data_buffer& partner_id);
    };
  }
}

#endif //__VDS_LOG_SYNC_SYNC_PROCESS_H_
