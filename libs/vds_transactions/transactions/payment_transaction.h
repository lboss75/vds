#ifndef __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
#define __VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class payment_transaction {
    public:
      static const transaction_id message_id = transaction_id::payment_transaction;

      payment_transaction(
        const const_data_buffer& source_transaction,
        const std::string& target_user,
        const uint64_t value)
        : source_transaction_(source_transaction),
          target_user_(target_user),
          value_(value) {
      }

      void serialize(binary_serializer & s) const {
        s << this->source_transaction_ << this->target_user_ << this->value_;
      }

      const const_data_buffer & source_transaction() const {
        return this->source_transaction_;
      }

      const std::string &target_user() const {
        return this->target_user_;
      }

      uint64_t value() const {
        return this->value_;
      }

    private:
      const_data_buffer source_transaction_;
      std::string target_user_;
      uint64_t value_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_PAYMENT_TRANSACTION_H_
