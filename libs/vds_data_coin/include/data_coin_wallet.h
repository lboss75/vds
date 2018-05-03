#ifndef __VDS_DATA_COIN_DATA_COIN_WALLET_H_
#define __VDS_DATA_COIN_DATA_COIN_WALLET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database.h"

namespace vds {
  namespace data_coin_private {
    class _wallet;
  }
}

namespace vds {
  namespace data_coin {
    class wallet {
    public:

      void save_transaction(
          database_transaction & t,
          const const_data_buffer & transaction_data);



    private:
      std::shared_ptr<data_coin_private::_wallet> impl_;
    };
  }
}

#endif // __VDS_DATA_COIN_DATA_COIN_WALLET_H_

