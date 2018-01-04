#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_DEPENDENCY_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_DEPENDENCY_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <cert_control.h>
#include "const_data_buffer.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "symmetriccrypto.h"
#include "guid.h"

namespace vds {
  namespace transactions {
    class channel_add_dependency_transaction {
    public:
      static const uint8_t category_id = transaction_log::transactions_category_id;
      static const uint8_t message_id = 'd';

      channel_add_dependency_transaction(
          const const_data_buffer & block_id,
          const const_data_buffer & key)
      : block_id_(block_id),
        key_(key){
      }

      void serialize(binary_serializer & s) const {
        s << this->block_id_ << this->key_;
      }

    private:
      const_data_buffer block_id_;
      const_data_buffer key_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_DEPENDENCY_TRANSACTION_H_
