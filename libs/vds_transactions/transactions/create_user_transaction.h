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
      std::shared_ptr<certificate> user_cert;
      std::string user_name;
      std::string parent_cert;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          user_credentials_key,
          user_cert,
          user_name,
          parent_cert
        );
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
