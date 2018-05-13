#ifndef __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
#define __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"

namespace vds {
  namespace transactions {
    class payment_transaction {
    public:

      const const_data_buffer & source_transaction() const {
        return this->source_transaction_;
      }

      const const_data_buffer &target_user() const {
        return this->target_user_;
      }

      uint64_t value() const {
        return this->value_;
      }

    private:
      const_data_buffer source_transaction_;
      const_data_buffer target_user_;
      uint64_t value_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
