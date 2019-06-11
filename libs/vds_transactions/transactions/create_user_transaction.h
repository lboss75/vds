#ifndef __VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_

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
    class create_user_transaction {
    public:
      static const transaction_id message_id = transaction_id::create_user_transaction;

      std::string user_credentials_key;
      std::shared_ptr<asymmetric_public_key> user_public_key;
      const_data_buffer user_private_key;
      std::string user_name;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          user_credentials_key,
          user_public_key,
          user_private_key,
          user_name
        );
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
