#ifndef __VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_
#define __VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <cert_control.h>
#include <channel_message_dbo.h>
#include "binary_serialize.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "transaction_log.h"
#include "transaction_id.h"

namespace vds {
  namespace transactions {

    class channel_message_transaction {
    public:
      static const uint8_t message_id = (uint8_t)transaction_id::channel_message_transaction;

      enum class channel_message_id : uint8_t {
        channel_add_writer_transaction,
        file_add_transaction
      };

      channel_message_transaction(binary_deserializer & s){
        s >> this->message_id_ >> this->channel_id_ >> this->read_cert_id_ >> this->write_cert_id_ >> this->data_;
      }

      binary_serializer & serialize(binary_serializer & s) const {
        return s << this->message_id_ << this->channel_id_ << this->read_cert_id_ << this->write_cert_id_ << this->data_;
      }

      void apply(
          const service_provider & sp,
          database_transaction & t) const{

        orm::channel_message_dbo t1;
        t.execute(t1.insert(
           t1.channel_id = this->channel_id_,
           t1.message_id = this->message_id_,
           t1.read_cert_id = this->read_cert_id_,
           t1.write_cert_id = this->write_cert_id_,
           t1.message = this->data_));
      }

    protected:
      uint8_t message_id_;
      guid channel_id_;
      guid read_cert_id_;
      guid write_cert_id_;
      const_data_buffer data_;

      channel_message_transaction(
          channel_message_id message_id,
          const guid & channel_id,
          const certificate &read_cert,
          const guid & write_cert_id,
          const asymmetric_private_key &write_cert_key,
          const const_data_buffer & data)
      : message_id_((uint8_t)message_id),
        channel_id_(channel_id),
        read_cert_id_(cert_control::get_id(read_cert)),
        write_cert_id_(write_cert_id){

        auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

        binary_serializer result;
        result
            << read_cert.public_key().encrypt(skey.serialize())
            << symmetric_encrypt::encrypt(skey, data);

        result << asymmetric_sign::signature(
            hash::sha256(),
            write_cert_key,
            (binary_serializer()
                << this->message_id_
                << this->channel_id_
                << this->read_cert_id_
                << this->write_cert_id_
                << result.data()).data());

        this->data_ = result.data();
      }
    };
  }
}

#endif //__VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_
