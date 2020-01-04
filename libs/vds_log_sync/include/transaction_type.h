#ifndef __VDS_TRANSACTIONS__TRANSACTION_TYPE_H_
#define __VDS_TRANSACTIONS__TRANSACTION_TYPE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace transactions {
    enum class transaction_type : uint8_t {
      user_transaction = 'u',
      node_transaction = 'n'
    };
  }
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_TYPE_H_
