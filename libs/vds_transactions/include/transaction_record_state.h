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
          const const_data_buffer & souce_account,
          const const_data_buffer & messages) {

        transaction_log::walk_messages(
            messages,
            [this, souce_account](payment_transaction & t){
              auto p = this->account_state_.find(souce_account);
              if(this->account_state_.end() == p){
                throw transaction_error("Transaction error");
              }

              if(p->second < t.value()){
                throw transaction_error("Transaction error");
              }

              p->second -= t.value();
              this->account_state_[t.target_user()] += t.value();
            }
        );
      }

    private:
      std::map<const_data_buffer, uint64_t> account_state_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
