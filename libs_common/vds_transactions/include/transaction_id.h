#ifndef __VDS_TRANSACTIONS__TRANSACTION_ID_H_
#define __VDS_TRANSACTIONS__TRANSACTION_ID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace transactions {
    enum class transaction_id : uint8_t {
      payment_transaction = 'p',
      payment_request_transaction = 'r',
      asset_issue_transaction = 'i',
      channel_message = 'c',
      create_user_transaction = 'u',
      node_add_transaction = 'm',
      create_wallet_transaction = 'w',
      store_block_transaction = 's',
      host_block_transaction = 'h'
    };
  }
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_ID_H_
