#ifndef __VDS_TRANSACTIONS_TRANSACTION_LOG_H_
#define __VDS_TRANSACTIONS_TRANSACTION_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
#include "const_data_buffer.h"
#include "transactions/root_user_transaction.h"
#include "transaction_log_record_dbo.h"
#include "transactions/transaction_messages_walker.h"

namespace vds {
  class transaction_log {
  public:

	  static void save(
		  const service_provider &sp,
		  class database_transaction &t,
		  const class const_data_buffer & block_id,
		  const class const_data_buffer & block_data);

    template <typename... handler_types>
    static void walk_messages(
      const const_data_buffer & message_data,
      handler_types && ... handlers) {

      transactions::transaction_messages_walker_lambdas<handler_types...> walker(
        std::forward<handler_types>(handlers)...);

      walker.process(message_data);
    }
  };

}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
