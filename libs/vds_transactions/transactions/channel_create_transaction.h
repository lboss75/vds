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
#include "transaction_id.h"

namespace vds {
  namespace transactions {
    class channel_create_transaction {
    public:
      static const uint8_t message_id = (uint8_t)transaction_id::channel_create_transaction;

      channel_create_transaction(
          const guid &channel_id,
          const std::string &name,
          const certificate &read_cert,
          const asymmetric_private_key &read_private_key,
          const certificate &write_cert,
          const asymmetric_private_key &write_private_key)
      : channel_id_(channel_id),
        name_(name),
        read_cert_(read_cert),
        read_private_key_(read_private_key),
        write_cert_(write_cert),
        write_private_key_(write_private_key) {
      }

      channel_create_transaction(binary_deserializer &s) {
        const_data_buffer read_private_key_der;
        const_data_buffer write_private_key_der;
        s
            >> this->channel_id_
            >> this->name_
            >> this->read_cert_
            >> read_private_key_der
            >> this->write_cert_
            >> write_private_key_der;

        this->read_private_key_ = asymmetric_private_key::parse_der(
            read_private_key_der, std::string());
        this->write_private_key_ = asymmetric_private_key::parse_der(
            write_private_key_der, std::string());
      }

      void serialize(binary_serializer & s) const {
        s
            << this->channel_id_
            << this->name_
            << this->read_cert_
            << read_private_key_.der(std::string())
            << this->write_cert_
            << write_private_key_.der(std::string());
      }

      const guid & channel_id() const {
        return channel_id_;
      }

      const std::string & name() const {
        return name_;
      }

      const certificate & read_cert() const {
        return read_cert_;
      }

      const asymmetric_private_key & read_private_key() const {
        return read_private_key_;
      }

      const certificate & write_cert() const {
        return write_cert_;
      }

      const asymmetric_private_key & write_private_key() const {
        return write_private_key_;
      }

      void apply(
          const service_provider & sp,
          database_transaction & t) {

      }

    private:
      guid channel_id_;
      std::string name_;
      certificate read_cert_;
      asymmetric_private_key read_private_key_;
      certificate write_cert_;
      asymmetric_private_key write_private_key_;
    };

  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
