#ifndef __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "asymmetriccrypto.h"
#include "binary_serialize.h"
#include "transaction_id.h"

namespace vds {
  class database_transaction;

  namespace transactions {
    class channel_create_transaction {
    public:
      static const channel_message_id message_id = channel_message_id::channel_create_transaction;

      const_data_buffer channel_id;
      std::string channel_type;
      std::string name;
      std::shared_ptr<asymmetric_public_key> read_public_key;
      std::shared_ptr<asymmetric_private_key> read_private_key;
      std::shared_ptr<asymmetric_public_key> write_public_key;
      std::shared_ptr<asymmetric_private_key> write_private_key;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          channel_id,
          channel_type,
          name,
          read_public_key,
          read_private_key,
          write_public_key,
          write_private_key
        );
      }

    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
