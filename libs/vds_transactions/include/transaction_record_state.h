#ifndef __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
#define __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include "payment_transaction.h"
#include "binary_serialize.h"
#include "transaction_log.h"


namespace vds {
  namespace transactions {
    class transaction_record_state {
    public:

      transaction_record_state() {        
      }

      transaction_record_state(transaction_record_state && original)
      : account_state_(std::move(original.account_state_)){
      }

      struct account_state_t {
        bool is_approved_;
        std::map<const_data_buffer/*source*/, uint64_t /*balance*/> balance_;

        account_state_t()
          : is_approved_(false) {          
        }
      };

      transaction_record_state(binary_deserializer & s){
        s >> this->account_state_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << this->account_state_;
        return s.data();
      }

      void apply(
        const transaction_block & block);

      bool update_consensus(const std::string & account) {
        auto p = this->account_state_.find(account);
        if(this->account_state_.end() == p || p->second.is_approved_) {
          return false;
        }

        p->second.is_approved_ = true;
        return true;
      }

      bool in_consensus() const {
        size_t approved = 0;
        size_t not_approved = 0;

        for(const auto & p : this->account_state_) {
          if(p.second.is_approved_) {
            ++approved;
          }
          else {
            ++not_approved;
          }
        }
        return (approved > not_approved);
      }

      void reset_consensus() {
        for (auto & p : this->account_state_) {
          p.second.is_approved_ = false;
        }
      }

    private:
      friend class data_coin_balance;

      std::map<std::string/*account*/, account_state_t> account_state_;
    };
  }
}

inline vds::binary_serializer & operator << (
  vds::binary_serializer & s,
  const vds::transactions::transaction_record_state::account_state_t & item) {
  s << item.is_approved_ << item.balance_;
  return s;
}

inline vds::binary_deserializer & operator >> (
  vds::binary_deserializer & s,
  vds::transactions::transaction_record_state::account_state_t & item) {
  s >> item.is_approved_ >> item.balance_;
  return s;
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
