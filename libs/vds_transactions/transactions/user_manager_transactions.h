#ifndef __VDS_TRANSACTIONS_USER_MANAGER_TRANSACTIONS_H_
#define __VDS_TRANSACTIONS_USER_MANAGER_TRANSACTIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "cert_control.h"
#include "types.h"
#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "database_orm.h"

namespace vds {
  namespace transactions {
    class root_user_transaction {
    public:
      static const transaction_id message_id = transaction_id::root_user_transaction;

      std::string user_credentials_key;
      std::shared_ptr<certificate> user_cert;
      const_data_buffer user_private_key;
      std::string user_name;

      template <typename visitor_t>
      void visit(visitor_t & v) {
        v(
          user_credentials_key,
          user_cert,
          user_name,
          user_private_key);
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_USER_MANAGER_TRANSACTIONS_H_
