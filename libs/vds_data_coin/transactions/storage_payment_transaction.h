#ifndef __VDS_DATA_COIN_STORAGE_PAYMENT_TRANSACTION_H_
#define __VDS_DATA_COIN_STORAGE_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "transactions/payment_transaction.h"

namespace vds {
  namespace data_coin {
    namespace transactions {
      class storage_payment_transaction : public payment_transaction {
      public:

      private:
        const_data_buffer storage_block_;
      };
    }
  }
}

#endif //__VDS_DATA_COIN_STORAGE_PAYMENT_TRANSACTION_H_
