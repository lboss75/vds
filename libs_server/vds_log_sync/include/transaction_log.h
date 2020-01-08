#ifndef __VDS_TRANSACTIONS_TRANSACTION_LOG_H_
#define __VDS_TRANSACTIONS_TRANSACTION_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "transaction_log_record_dbo.h"

namespace vds {
  class database_read_transaction;
  class database_transaction;

  namespace transactions {
    class transaction_block;
    class payment_transaction;
    class channel_message;
    class create_user_transaction;
    class node_add_transaction;
    class create_wallet_transaction;
    class asset_issue_transaction;
    class store_block_transaction;

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
        const const_data_buffer & block_id,
        bool value);

    private:
      static expected<void> process_block_with_followers(
        const service_provider * sp,
        class database_transaction &t,
        const const_data_buffer & block_id,
        const const_data_buffer & block_data,
        orm::transaction_log_record_dbo::state_t state,
        bool in_consensus);

      static expected<orm::transaction_log_record_dbo::state_t> process_block(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block,
        bool in_consensus);

      static expected<void> process_followers(
        const service_provider * sp,
        class database_transaction &t);

      //returns false to stop process block
      static expected<bool> update_consensus(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block,
        const const_data_buffer & block_data,
        orm::transaction_log_record_dbo::state_t state,
        bool in_consensus);

      static expected<void> rollback_all(
        const service_provider* sp,
        database_transaction& t,
        uint64_t min_order);


      static expected<bool> process_records(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block);

      static expected<void> consensus_records(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block);

      static expected<void> undo_records(
        const service_provider * sp,
        class database_transaction &t,
        const transaction_block & block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const payment_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const payment_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const payment_transaction & message,
        const transaction_block& block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const channel_message & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const channel_message & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const channel_message & message,
        const transaction_block& block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_user_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_user_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        database_transaction &t,
        const create_user_transaction & message,
        const transaction_block& block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const node_add_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const node_add_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const node_add_transaction & message,
        const transaction_block& block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_wallet_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_wallet_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const create_wallet_transaction & message,
        const transaction_block& block);

      ////////////
      static expected<bool> apply_record(
        const service_provider * sp,
        class database_transaction &t,
        const asset_issue_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider * sp,
        class database_transaction &t,
        const asset_issue_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider * sp,
        class database_transaction &t,
        const asset_issue_transaction & message,
        const transaction_block& block);
      //////////// store_block_transaction
      static expected<bool> apply_record(
        const service_provider* sp,
        class database_transaction& t,
        const store_block_transaction & message,
        const transaction_block& block);

      static expected<void> consensus_record(
        const service_provider* sp,
        class database_transaction& t,
        const store_block_transaction & message,
        const transaction_block& block);

      static expected<void> undo_record(
        const service_provider* sp,
        class database_transaction& t,
        const store_block_transaction& message,
        const transaction_block& block);
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_LOG_H_
