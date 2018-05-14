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

      bool apply(
          const const_data_buffer & souce_account,
          const const_data_buffer & transaction_id,
          const const_data_buffer & messages) {

        bool result = true;
        transaction_log::walk_messages(
            messages,
            [&result, this, souce_account, transaction_id](payment_transaction & t)->bool{
              auto p = this->account_state_.find(souce_account);
              if(this->account_state_.end() == p){
                result = false;
                return false;
              }

              auto p1 = p->second.find(t.source_transaction());
              if(p->second.end() == p1){
                result = false;
                return false;
              }

              if(p1->second < t.value()){
                result = false;
                return false;
              }

              p1->second -= t.value();
              this->account_state_[t.target_user()][transaction_id] += t.value();
              return true;
            }
        );

        return result;
      }

    private:
      std::map<const_data_buffer, std::map<const_data_buffer, uint64_t>> account_state_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
