#ifndef __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_
#define __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "transaction_record_state.h"

namespace vds {
  namespace transactions {
    class data_coin_balance {
    public:
      data_coin_balance()
      : order_no_(1) {        
      }

      data_coin_balance(vds::transactions::data_coin_balance &&original)
      : order_no_(original.order_no_), state_(std::move(original.state_)) {        
      }

      data_coin_balance(
          const const_data_buffer & root_block_id,
          const std::string & root_account)
          : order_no_(1), state_(transaction_record_state(root_block_id, root_account)) {
      }

      uint64_t order_no() const {
        return this->order_no_;
      }

      static data_coin_balance load(
        database_transaction & t,
        std::set<vds::const_data_buffer> & base_packages);

      const_data_buffer serialize() const {
        return this->state_.serialize();
      }

      data_coin_balance & operator = (data_coin_balance && original){
        this->order_no_ = original.order_no_;
        this->state_ = std::move(original.state_);
        return *this;
      }

    private:
      uint64_t order_no_;
      transaction_record_state state_;

      data_coin_balance(
        uint64_t order_no,
        transaction_record_state && state)
        : order_no_(order_no),
          state_(std::move(state)) {
      }
    };
  }
}

#endif // __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_

