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

      static bool save(
        const service_provider *sp,
        class database_transaction &t,
        const const_data_buffer & block_data);

    private:
      static void update_consensus(
        database_transaction& t,
        const transaction_block& block,
        const std::string & account);

      static bool process_block(
        const service_provider *sp,
        class database_transaction &t,
        const const_data_buffer & block_data,
        bool saved);
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
