#ifndef __VDS_TRANSACTIONS_TRANSACTION_LOG_H_
#define __VDS_TRANSACTIONS_TRANSACTION_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "user_manager_transactions.h"

namespace vds {
  namespace transactions {
    class transaction_block;

    class transaction_log {
    public:

      static expected<const_data_buffer> save(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & block_data);

      static expected<bool> check_consensus(
        database_read_transaction& t,
        const const_data_buffer & log_id);

      static expected<void> invalid_block(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & block_id);

    private:
      static expected<void> process_block(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & block_data);

      static expected<void> invalid_become_consensus(
        const service_provider* sp,
        const database_transaction& t,
        const const_data_buffer &  log_id);

      static expected<void> make_consensus(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & log_id);

      static expected<void> update_consensus(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & block_data);
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
