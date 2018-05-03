#ifndef __VDS_DATA_COIN_DATA_COIN_BALANCE_H_
#define __VDS_DATA_COIN_DATA_COIN_BALANCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "coin_transaction_package.h"

namespace vds {
  namespace data_coin_private {
    class data_coin_balance {
    public:
      struct account_state_t {

      };

      data_coin_balance();
      data_coin_balance(const const_data_buffer & data);
      data_coin_balance(data_coin_balance && original);

      static data_coin_balance load(
          database_transaction & t,
          const std::list<const_data_buffer> & base_packages);

    private:
      std::map<const_data_buffer, account_state_t> accounts_;

      void apply(
          const const_data_buffer & source_user,
          const data_coin::transactions::payment_transaction &transaction);
    };
  }
}

#endif // __VDS_DATA_COIN_DATA_COIN_BALANCE_H_

