#ifndef __VDS_DATA_COIN_DATA_COIN_WALLET_P_H_
#define __VDS_DATA_COIN_DATA_COIN_WALLET_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "coin_transaction_package.h"

namespace vds {
  namespace data_coin_private {
    class _wallet : public std::enable_shared_from_this<_wallet> {
    public:

      void save_transaction(
        database_transaction & t,
        const const_data_buffer & transaction_data);

    private:
      std::set<const_data_buffer> applied_transactions_;
      data_coin_private::coin_transaction_package current_transaction_;




      data_coin::coin_transaction_package lookup_common_base(
        database_transaction & t,
        const data_coin::coin_transaction_package & left_package,
        const data_coin::coin_transaction_package & right_package);

    };
  }
}

#endif // __VDS_DATA_COIN_DATA_COIN_WALLET_H_

