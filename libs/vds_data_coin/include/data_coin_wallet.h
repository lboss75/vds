#ifndef __VDS_DATA_COIN_DATA_COIN_WALLET_H_
#define __VDS_DATA_COIN_DATA_COIN_WALLET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace data_coin_private {
    class _wallet;
  }
}

namespace vds {
  namespace data_coin {
    class wallet {
    public:


    private:
      std::shared_ptr<data_coin_private::_wallet> impl_;
    };
  }
}

#endif // __VDS_DATA_COIN_DATA_COIN_WALLET_H_

