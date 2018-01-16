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
      channel_add_writer_transaction(
          const guid & channel_id,
          const certificate & target_cert,
          const guid & sing_cert_id,
          const asymmetric_private_key & sing_cert_private_key,

          const certificate & read_cert,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key)
      : channel_message_transaction(
          channel_message_id::channel_add_writer_transaction,
          channel_id,
          target_cert,
          sing_cert_id,
          sing_cert_private_key,
          (
            binary_serializer()
                << read_cert.der()
                << write_cert.der()
                << write_private_key.der(std::string())
          ).data()) {
      }

      channel_add_writer_transaction(binary_deserializer & s)
          : channel_message_transaction(s){
      }

      static void apply_message(
          const certificate &device_cert,
          const service_provider &sp,
          database_transaction &t,
          binary_deserializer &data_stream) {
          const_data_buffer read_cert_der;
          const_data_buffer write_cert_der;
          const_data_buffer write_private_key_der;

          data_stream >> read_cert_der >> write_cert_der >> write_private_key_der;

        auto read_cert = certificate::parse_der(read_cert_der);
        auto write_cert = certificate::parse_der(write_cert_der);
        auto write_private_key = asymmetric_private_key::parse_der(
            write_private_key_der,
            std::string());

      }
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
