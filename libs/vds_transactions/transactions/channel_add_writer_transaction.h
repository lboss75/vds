#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_control.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"


namespace vds {
  namespace transactions {
    class channel_add_writer_transaction  {
    public:
			static const channel_message_id message_id = channel_message_id::channel_add_writer_transaction;

      const_data_buffer id;
      std::string channel_type;
      std::string name;
      std::shared_ptr<asymmetric_public_key> read_cert;
      std::shared_ptr<asymmetric_private_key> read_private_key;
      std::shared_ptr<asymmetric_public_key> write_cert;
      std::shared_ptr<asymmetric_private_key> write_private_key;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          id,
          channel_type,
          name,
          read_cert,
          read_private_key,
          write_cert,
          write_private_key
        );
      }

    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
