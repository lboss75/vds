#ifndef __VDS_DATA_COIN_PAYMENT_TRANSACTION_H_
#define __VDS_DATA_COIN_PAYMENT_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"

namespace vds {
  namespace data_coin {
    namespace transactions {
      class payment_transaction {
      public:

      private:
        const_data_buffer target_user_certificate_thumbprint_;
        uint64_t value_;
      };
    }
  }
}

#endif //__VDS_DATA_COIN_PAYMENT_TRANSACTION_H_
