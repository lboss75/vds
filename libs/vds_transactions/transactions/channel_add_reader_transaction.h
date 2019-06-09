#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "asymmetriccrypto.h"
#include "channel_message_id.h"

namespace vds {
  namespace transactions {
    class channel_add_reader_transaction {
    public:
			static const channel_message_id message_id = channel_message_id::channel_add_reader_transaction;

      const_data_buffer id;
      std::string channel_type;
      std::string name;
      std::shared_ptr<asymmetric_public_key> read_cert;
      std::shared_ptr<asymmetric_private_key> read_private_key;
      std::shared_ptr<asymmetric_public_key> write_cert;

      template <typename  visitor_type>
      auto & visit(visitor_type & v) {
        return v(
          id,
          channel_type,
          name,
          read_cert,
          read_private_key,
          write_cert
        );
      }
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
