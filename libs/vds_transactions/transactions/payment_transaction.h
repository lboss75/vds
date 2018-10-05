#ifndef __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
#define __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "binary_serialize.h"
#include "const_data_buffer.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class payment_transaction {
    public:
      static const transaction_id message_id = transaction_id::payment_transaction;

      const_data_buffer source_transaction;
      std::string target_user;
      uint64_t value;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          source_transaction,
          target_user,
          value);
      }

    };
  }
}

#endif //__VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
