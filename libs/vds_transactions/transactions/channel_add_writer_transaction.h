#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_control.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "symmetriccrypto.h"
#include "channel_message_transaction.h"

namespace vds {
  namespace transactions {
    class channel_add_writer_transaction : public channel_message_transaction {
    public:
      static const uint8_t channel_message_id = 'w';

      channel_add_writer_transaction(
          const guid & channel_id,
          const certificate & owner_cert,

          const certificate & read_cert,
          const asymmetric_private_key & read_private_key,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key)
      : channel_message_transaction(
          channel_id,
          read_cert,
          write_private_key,
          (
            binary_serializer()
                << channel_message_id
                << read_cert.der()
                << read_private_key.der(std::string())
                << write_cert.der()
                << write_private_key.der(std::string())
          ).data()) {
      }

      channel_add_writer_transaction(binary_deserializer & s)
          : channel_message_transaction(s){
      }
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
