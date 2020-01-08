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

namespace vds {
  namespace transactions {
    class create_user_transaction {
    public:
      static const transaction_id message_id = transaction_id::create_user_transaction;

      std::string user_email;
      std::shared_ptr<asymmetric_public_key> user_public_key;
      const_data_buffer user_profile_id;
      std::string user_name;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          user_email,
          user_public_key,
          user_profile_id,
          user_name
        );
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
