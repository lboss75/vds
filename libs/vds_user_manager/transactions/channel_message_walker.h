#ifndef __VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_
#define __VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <map>
#include "asymmetriccrypto.h"
#include "binary_serialize.h"
#include "symmetriccrypto.h"
#include "channel_add_message_transaction.h"

namespace vds {
  template <typename implementation_class>
  class channel_message_walker {
  public:

    void apply(
        const certificate & cert,
        const asymmetric_private_key & cert_key,
        const const_data_buffer & message){

      const_data_buffer key_crypted;
      const_data_buffer data_crypted;
      binary_deserializer s(message);
      s
          >> key_crypted
          >> data_crypted;

      auto body_size = message.size() - s.size();

      const_data_buffer signature;
      s >> signature;

      if(!asymmetric_sign_verify::verify(
          hash::sha256(),
          cert.public_key(),
          signature,
          message.data(),
          body_size)){
        throw std::runtime_error("Data is corrupted");
      }

      auto key_data = cert_key.decrypt(key_crypted);
      auto skey = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_data));

      auto message_data = symmetric_decrypt::decrypt(skey, data_crypted);

      binary_deserializer message_stream(message_data);
      uint8_t  message_id;
      message_stream >> message_id;
      switch(message_id){
        case channel_add_message_transaction::create_channel::message_id:
        {
          static_cast<implementation_class *>(this)->apply(
              channel_add_message_transaction::create_channel(message_stream));
          break;
        }
        case channel_add_message_transaction::add_device_user::message_id:
        {
          static_cast<implementation_class *>(this)->apply(
              channel_add_message_transaction::add_device_user(message_stream));
          break;
        }
      }

      if(0 != message_stream.size()){
        throw std::runtime_error("Data is corrupted");
      }
    }

    void apply(const channel_add_message_transaction::create_channel & message){
    }

    void apply(const channel_add_message_transaction::add_device_user & message){
    }
  };

  class channel_collect_certificate : public channel_message_walker<channel_collect_certificate> {
  public:

    void apply(const channel_add_message_transaction::create_channel & message){
    }

    void apply(const channel_add_message_transaction::add_device_user & message){

    }

  private:
    struct cert_info
    {
      certificate cert;
      asymmetric_private_key cert_key;
    };

    std::map<guid, cert_info> certificates_;

  };
}

#endif //__VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_

