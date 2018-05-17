#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "guid.h"
#include "asymmetriccrypto.h"
#include "channel_message_id.h"

namespace vds {
  namespace transactions {
    class channel_add_reader_transaction {
    public:
			static const channel_message_id message_id = channel_message_id::channel_add_reader_transaction;

      channel_add_reader_transaction(
        const std::string & name,
        const certificate & read_cert,
        const asymmetric_private_key & read_private_key,
        const certificate & write_cert)
      : name_(name),
        read_cert_(read_cert),
        read_private_key_(read_private_key),
        write_cert_(write_cert) {
      }

      channel_add_reader_transaction(binary_deserializer & s){
        const_data_buffer read_private_key_der;
        s
            >> this->name_
            >> this->read_cert_
            >> read_private_key_der
            >> this->write_cert_;
        this->read_private_key_ = asymmetric_private_key::parse_der(
            read_private_key_der,
            std::string());
      }

      void serialize(binary_serializer & s){
        s
            << this->name_
            << this->read_cert_
            << this->read_private_key_.der(std::string())
            << this->write_cert_;
      }

      const std::string & name() const {
        return name_;
      }

      const certificate &read_cert() const {
        return read_cert_;
      }

      const asymmetric_private_key &read_private_key() const {
        return read_private_key_;
      }

      const certificate & write_cert() const {
        return write_cert_;
      }

    private:
      std::string name_;
      certificate read_cert_;
      asymmetric_private_key read_private_key_;
      certificate write_cert_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
