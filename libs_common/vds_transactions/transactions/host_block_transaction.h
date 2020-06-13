#ifndef __VDS_TRANSACTIONS_HOST_BLOCK_TRANSACTION_H_
#define __VDS_TRANSACTIONS_HOST_BLOCK_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class host_block_transaction {
    public:
      static const transaction_id message_id = transaction_id::host_block_transaction;

      const_data_buffer replica_hash;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          replica_hash
        );
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_HOST_BLOCK_TRANSACTION_H_
