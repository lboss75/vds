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
        enum class payment_type_t {
          fiat,
          storage
        };

        payment_type_t payment_type() const {
          return this->payment_type_;
        }

        const const_data_buffer & target_user_certificate_thumbprint() const {
          return this->target_user_certificate_thumbprint_;
        }

        uint64_t value() const {
          return this->value_;
        }

      private:
        payment_type_t payment_type_;
        const_data_buffer target_user_certificate_thumbprint_;
        uint64_t value_;
      };
    }
  }
}

#endif //__VDS_DATA_COIN_PAYMENT_TRANSACTION_H_
