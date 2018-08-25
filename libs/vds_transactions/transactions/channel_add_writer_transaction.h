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

      channel_add_writer_transaction(
        const const_data_buffer & id,
        const std::string & channel_type,
        const std::string & name,
        const certificate & read_cert,
        const asymmetric_private_key & read_private_key,
        const certificate & write_cert,
        const asymmetric_private_key & write_private_key)
        : id_(id),
        channel_type_(channel_type),
        name_(name),
        read_cert_(read_cert),
        read_private_key_(read_private_key),
        write_cert_(write_cert),
        write_private_key_(write_private_key) {
      }

      channel_add_writer_transaction(binary_deserializer & s) {
        const_data_buffer write_private_key_der;
        s
          >> this->id_
          >> this->channel_type_
            >> this->name_
            >> this->read_cert_
            >> this->write_cert_
            >> write_private_key_der;

        this->write_private_key_ = asymmetric_private_key::parse_der(
            write_private_key_der,
            std::string());
      }

      void serialize(binary_serializer & s) const {
        s
          << this->id_
          << this->channel_type_
            << this->name_
            << this->read_cert_
            << this->write_cert_
            << this->write_private_key_.der(std::string());
      }

      const const_data_buffer & id() const {
        return this->id_;
      }

      const std::string & channel_type() const {
        return this->channel_type_;
      }

      const std::string &name() const {
        return this->name_;
      }

      const certificate &read_cert() const {
        return this->read_cert_;
      }

      const asymmetric_private_key & read_private_key() const {
        return this->read_private_key_;
      }

      const certificate &write_cert() const {
        return this->write_cert_;
      }

      const asymmetric_private_key &write_private_key() const {
        return this->write_private_key_;
      }

    private:
      const_data_buffer id_;
      std::string channel_type_;
      std::string name_;
      certificate read_cert_;
      asymmetric_private_key read_private_key_;
      certificate write_cert_;
      asymmetric_private_key write_private_key_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
