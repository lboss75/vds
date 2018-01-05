#ifndef __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "binary_serialize.h"

namespace vds {
  namespace transactions {
    class channel_create_transaction {
    public:
      static const uint8_t category_id = transaction_log::transactions_category_id;
      static const uint8_t message_id = 'c';

      enum class channel_type : uint8_t {
        normal,
        user
      };

      channel_create_transaction(
          channel_type type,
          const guid &channel_id,
          const std::string &name,
          const certificate &read_cert,
          const asymmetric_private_key &read_cert_key,
          const certificate &write_cert,
          const asymmetric_private_key &write_key);

      channel_create_transaction(
          channel_type type,
          const guid &channel_id,
          const certificate &read_cert,
          const asymmetric_private_key &read_cert_key);

      channel_create_transaction(binary_deserializer & s){
        const_data_buffer read_cert_der;
        const_data_buffer read_cert_key_der;
        const_data_buffer write_cert_der;
        const_data_buffer write_key_der;

        s
            >> this->channel_id_
            >> this->name_
            >> read_cert_der
            >> read_cert_key_der
            >> write_cert_der
            >> write_key_der;

        this->read_cert_ = certificate::parse_der(read_cert_der);
        this->read_cert_key_ = asymmetric_private_key::parse_der(read_cert_key_der, std::string());

        this->write_cert_ = certificate::parse_der(write_cert_der);
        this->write_key_ = asymmetric_private_key::parse_der(write_key_der, std::string());
      }

      binary_serializer & serialize(binary_serializer & s) const {
        s
            << this->channel_id_
            << this->name_
            << this->read_cert_.der()
            << this->read_cert_key_.der("")
            << this->write_cert_.der()
            << this->write_key_.der("");
        return  s;
      }
    private:
      guid channel_id_;
      std::string name_;
      certificate read_cert_;
      asymmetric_private_key read_cert_key_;
      certificate write_cert_;
      asymmetric_private_key write_key_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
