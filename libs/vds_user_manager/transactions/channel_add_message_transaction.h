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
      s << message_t::message_id;
      message.serialize(s);

      auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

      binary_serializer result;
      result
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
      static constexpr uint8_t message_id = 'c';

      create_channel(const guid &channel_id, const guid &read_cert_id, const certificate &read_cert,
                   const asymmetric_private_key &read_cert_key, const guid &write_cert_id, const certificate &write_cert,
                   const asymmetric_private_key &write_key);

      create_channel(binary_deserializer & s){
        const_data_buffer read_cert_der;
        const_data_buffer read_cert_key_der;
        const_data_buffer write_cert_der;
        const_data_buffer write_key_der;

        s
            >> this->channel_id_
            >> this->read_cert_id_
            >> read_cert_der
            >> read_cert_key_der
            >> this->write_cert_id_
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
            << this->read_cert_id_
            << this->read_cert_.der()
            << this->read_cert_key_.der("")
            << this->write_cert_id_
            << this->write_cert_.der()
            << this->write_key_.der("");
        return  s;
      }
    private:
      guid channel_id_;
      guid read_cert_id_;
      certificate read_cert_;
      asymmetric_private_key read_cert_key_;
      guid write_cert_id_;
      certificate write_cert_;
      asymmetric_private_key write_key_;
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
