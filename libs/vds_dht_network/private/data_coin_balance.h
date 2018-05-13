#ifndef __VDS_DHT_NETWORK_DATA_COIN_BALANCE_H_
#define __VDS_DHT_NETWORK_DATA_COIN_BALANCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "include/transaction_record_state.h"

namespace vds {
  namespace dht_network_private {
    class data_coin_balance {
    public:

      data_coin_balance();
      data_coin_balance(const const_data_buffer & data);
      data_coin_balance(data_coin_balance && original);

      static data_coin_balance load(
          database_transaction & t,
          const std::list<const_data_buffer> & base_packages);

    private:
      transactions::transaction_record_state state_;

      void apply(
          const const_data_buffer & source_user,
          const data_coin::transactions::payment_transaction &transaction);

    };
  }
}

#endif // __VDS_DHT_NETWORK_DATA_COIN_BALANCE_H_

