#ifndef __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
#define __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <transactions/payment_transaction.h>
#include "binary_serialize.h"
#include "include/transaction_log.h"
#include "transaction_error.h"

namespace vds {
  namespace transactions {
    class transaction_record_state {
    public:
      transaction_record_state(binary_deserializer & s){
        s >> this->account_state_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << this->account_state_;
        return s.data();
      }

      void apply(
        const std::string & souce_account,
        const const_data_buffer & transaction_id,
        const const_data_buffer & messages);

    private:
      std::map<std::string, std::map<const_data_buffer, uint64_t>> account_state_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
