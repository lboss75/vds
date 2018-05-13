#ifndef __VDS_DATA_COIN_TRANSACTION_VISITER_H_
#define __VDS_DATA_COIN_TRANSACTION_VISITER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "../../vds_transactions/transactions/payment_transaction.h"
#include "transactions/storage_payment_transaction.h"

namespace vds {
  namespace data_coin {
    namespace transactions {
      template <typename implementation_class>
      class transaction_visiter {
      public:

        bool visit_payment_transaction(const payment_transaction & transaction){
          return true;
        }

        bool visit_storage_payment_transaction(const storage_payment_transaction & transaction){
          return true;
        }

      };
    }
  }
}

#endif //__VDS_DATA_COIN_TRANSACTION_VISITER_H_
