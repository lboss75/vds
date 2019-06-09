#ifndef __VDS_TRANSACTIONS_TRANSACTION_LOG_H_
#define __VDS_TRANSACTIONS_TRANSACTION_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"

namespace vds {
  class database_read_transaction;
  class database_transaction;

  namespace transactions {
    class transaction_block;
    class payment_transaction;
    class channel_message;
    class create_user_transaction;
    class node_add_transaction;

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

      static expected<bool> process_records(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const payment_transaction & message,
        const const_data_buffer & block_id);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const payment_transaction & message,
        const const_data_buffer & block_id);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const channel_message & message,
        const const_data_buffer & block_id);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const channel_message & message,
        const const_data_buffer & block_id);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_user_transaction & message,
        const const_data_buffer & block_id);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_user_transaction & message,
        const const_data_buffer & block_id);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const node_add_transaction & message,
        const const_data_buffer & block_id);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const node_add_transaction & message,
        const const_data_buffer & block_id);

    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
