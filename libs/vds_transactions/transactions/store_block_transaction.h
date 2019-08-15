#ifndef __VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_
#define __VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "database_orm.h"

namespace vds {
  namespace transactions {
    class store_block_transaction {
    public:
      static const transaction_id message_id = transaction_id::store_block_transaction;

      const_data_buffer owner_id;
      const_data_buffer object_id;
      uint64_t object_size;
      const_data_buffer owner_sig;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          owner_id,
          object_id,
          object_size,
          owner_sig
        );
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_STORE_BLOCK_TRANSACTION_H_
