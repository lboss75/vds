#ifndef __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_
#define __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "include/transaction_record_state.h"

namespace vds {
  namespace transactions {
    class data_coin_balance {
    public:

      data_coin_balance();
      data_coin_balance(const const_data_buffer & data);
      data_coin_balance(data_coin_balance && original);

      static data_coin_balance load(
        database_transaction & t);

    private:
      uint64_t order_no_;
      transaction_record_state state_;

      data_coin_balance(uint64_t order_no, transaction_record_state && state);

    };
  }
}

#endif // __VDS_TRANSACTIONS_DATA_COIN_BALANCE_H_

