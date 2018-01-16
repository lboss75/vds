#ifndef __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <certificate_dbo.h>
#include <channel_dbo.h>
#include <certificate_private_key_dbo.h>
#include "types.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "channel_message_transaction.h"

namespace vds {
  namespace transactions {
    class channel_create_transaction : public channel_message_transaction {
    public:
      channel_create_transaction(
          const guid &owner_id,
          const certificate &target_cert,
          const guid &write_cert_id,
          const asymmetric_private_key &write_cert_key,
          const guid &new_channel_id,
          const std::string &name,
          const certificate &read_cert,
          const asymmetric_private_key &read_private_key,
          const certificate &write_cert,
          const asymmetric_private_key &write_private_key)
          : channel_message_transaction(
          channel_message_id::channel_create_transaction,
          owner_id,
          target_cert,
          write_cert_id,
          write_cert_key,
          (binary_serializer()
              << new_channel_id
              << name
              << read_cert.der()
              << read_private_key.der(std::string())
              << write_cert.der()
              << write_private_key.der(std::string())).data()) {
      }

      channel_create_transaction(binary_deserializer &s)
          : channel_message_transaction(s) {
      }

      template<typename target>
      static void parse_message(binary_deserializer &s, target t) {
        guid channel_id;
        std::string name;
        const_data_buffer read_cert_data;
        const_data_buffer read_private_key_data;
        const_data_buffer write_cert_data;
        const_data_buffer write_private_key_data;

        s
            >> channel_id
            >> name
            >> read_cert_data
            >> read_private_key_data
            >> write_cert_data
            >> write_private_key_data;

        t(
            channel_id,
            name,
            certificate::parse_der(read_cert_data),
            asymmetric_private_key::parse_der(read_private_key_data, std::string()),
            certificate::parse_der(write_cert_data),
            asymmetric_private_key::parse_der(write_private_key_data, std::string()));
      }

      static void apply_message(
          const certificate &owner_cert,
          const service_provider &sp,
          database_transaction &t,
          binary_deserializer &s) {
        parse_message(s, [&owner_cert, &sp, &t](
            const guid &channel_id,
            const std::string &name,
            const certificate &read_cert,
            const asymmetric_private_key &read_private_key,
            const certificate &write_cert,
            const asymmetric_private_key &write_private_key) {

          sp.get<logger>()->trace(
              "TRLOG",
              sp,
              "Create channel %s",
              channel_id.str().c_str());

          dbo::certificate t1;
          t.execute(t1.insert(
              t1.id = cert_control::get_id(read_cert),
              t1.cert = read_cert.der(),
              t1.parent = cert_control::get_parent_id(read_cert)));

          dbo::certificate_private_key t2;
          t.execute(t2.insert(
              t2.id = cert_control::get_id(read_cert),
              t2.owner_id = cert_control::get_id(owner_cert),
              t2.body = owner_cert.public_key().encrypt(read_private_key.der(std::string()))));

          t.execute(t1.insert(
              t1.id = cert_control::get_id(write_cert),
              t1.cert = write_cert.der(),
              t1.parent = cert_control::get_parent_id(write_cert)));

          t.execute(t2.insert(
              t2.id = cert_control::get_id(write_cert),
              t2.owner_id = cert_control::get_id(owner_cert),
              t2.body = owner_cert.public_key().encrypt(write_private_key.der(std::string()))));

          dbo::channel t3;
          t.execute(t3.insert(
              t3.id = channel_id,
              t3.name = name,
              t3.read_cert = cert_control::get_id(read_cert),
              t3.write_cert = cert_control::get_id(write_cert)));
        });
      }
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
