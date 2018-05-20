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
      channel_message = 'c',
      root_user_transaction = 'r'
    };
  }
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_ID_H_
