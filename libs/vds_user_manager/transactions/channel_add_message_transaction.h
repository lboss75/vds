#ifndef __VDS_USER_MANAGER_CHANNEL_ADD_MESSAGE_TRANSACTION_H_
#define __VDS_USER_MANAGER_CHANNEL_ADD_MESSAGE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "binary_serialize.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "transaction_log.h"

namespace vds {
  class channel_add_message_transaction {
  public:
    static const uint8_t category_id = transaction_log::user_manager_category_id;
    static const uint8_t message_id = 'm';

    template <typename message_t>
    channel_add_message_transaction(
        const guid & cert_id,
        const certificate & cert,
        const asymmetric_private_key & cert_key,
        message_t && message)
    : cert_id_(cert_id) {

      binary_serializer s;
      message.serialize(s);

      auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

      binary_serializer result;
      result
          << cert_id
          << cert.public_key().encrypt(skey.serialize())
          << symmetric_encrypt::encrypt(skey, s.data());

      result << asymmetric_sign::signature(
          hash::sha256(),
          cert_key,
          result.data());

      this->data_ = result.data();
    }

    const guid & cert_id() const { return this->cert_id_; }
    const guid & channel_id() const { return this->channel_id_; }
    const const_data_buffer & message() const { return this->data_; }

    class create_channel {
    public:

      create_channel(const guid &channel_id, const guid &read_cert_id, const certificate &read_cert,
                   const asymmetric_private_key &read_cert_key, const guid &write_cert_id, const certificate &write_cert,
                   const asymmetric_private_key &write_key);
    };

    channel_add_message_transaction(binary_deserializer & s){
      s >> this->cert_id_ >> this->channel_id_ >> this->data_;
    }

    binary_serializer & serialize(binary_serializer & s) const {
      s << this->cert_id_ << this->channel_id_ << this->data_;
      return  s;
    }

  private:
    guid cert_id_;
    guid channel_id_;
    const_data_buffer data_;

  };
}

#endif //__VDS_USER_MANAGER_CHANNEL_ADD_MESSAGE_TRANSACTION_H_
