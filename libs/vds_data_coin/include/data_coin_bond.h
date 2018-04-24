#ifndef __VDS_DATA_COIN_DATA_COIN_BOND_H_
#define __VDS_DATA_COIN_DATA_COIN_BOND_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace data_coin {
    class bond {
    public:

    private:
      certificate issuer_;
      uint64_t value_;
    };
  }
}

#endif // __VDS_DATA_COIN_DATA_COIN_BOND_H_

