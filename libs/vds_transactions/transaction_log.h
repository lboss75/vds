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

namespace vds {
  class transaction_log {
  public:

	  static void save(
		  const service_provider &sp,
		  class database_transaction &t,
		  const class const_data_buffer & block_id,
		  const class const_data_buffer & block_data);

	  static void apply_block(
		  const service_provider & sp,
		  class database_transaction &t,
		  const const_data_buffer & block_id);
  private:

    static orm::transaction_log_record_dbo::state_t apply_block(
        const service_provider &sp,
        class database_transaction &t,
        const class const_data_buffer & block_id,
        const class const_data_buffer & block_data,
        std::list<const_data_buffer> & followers);

    static void apply_message(
				const vds::service_provider &sp,
        database_transaction &t,
        const const_data_buffer &block_data);

  };

}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
