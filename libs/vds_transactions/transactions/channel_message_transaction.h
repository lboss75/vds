#ifndef __VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_
#define __VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <cert_control.h>
#include "binary_serialize.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "transaction_log.h"

namespace vds {
  namespace transactions {

    class channel_message_transaction {
    public:
      static const uint8_t message_id = 'm';

      channel_message_transaction(binary_deserializer & s){
        s >> this->channel_id_ >> this->data_;
      }


      binary_serializer & serialize(binary_serializer & s) const {
        return s << this->channel_id_ << this->data_;
      }

    protected:
      guid channel_id_;
      const_data_buffer data_;

      channel_message_transaction(
          const guid & channel_id,
          const certificate &read_cert,
          const asymmetric_private_key &write_cert_key,
          const const_data_buffer & data)
      : channel_id_(channel_id) {

        auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

        binary_serializer result;
        result
            << cert_control::get_id(read_cert)
            << read_cert.public_key().encrypt(skey.serialize())
            << symmetric_encrypt::encrypt(skey, data);

        result << asymmetric_sign::signature(
            hash::sha256(),
            write_cert_key,
            result.data());

        this->data_ = result.data();
      }
    };
  }
}

#endif //__VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_
