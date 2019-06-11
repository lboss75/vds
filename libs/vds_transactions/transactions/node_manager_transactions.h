#ifndef __VDS_TRANSACTIONS_NODE_MANAGER_TRANSACTIONS_H_
#define __VDS_TRANSACTIONS_NODE_MANAGER_TRANSACTIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "keys_control.h"
#include "types.h"
#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "database_orm.h"

namespace vds {
  namespace transactions {
    class node_add_transaction {
    public:
      static const transaction_id message_id = transaction_id::node_add_transaction;

      std::shared_ptr<asymmetric_public_key> node_public_key;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(node_public_key);
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_NODE_MANAGER_TRANSACTIONS_H_
