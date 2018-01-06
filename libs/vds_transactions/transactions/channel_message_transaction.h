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

    protected:
      const_data_buffer data_;

      channel_message_transaction(
          const certificate &cert,
          const asymmetric_private_key &cert_key,
          const const_data_buffer & data) {

        auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

        binary_serializer result;
        result
            << cert_control::get_id(cert)
            << cert.public_key().encrypt(skey.serialize())
            << symmetric_encrypt::encrypt(skey, data);

        result << asymmetric_sign::signature(
            hash::sha256(),
            cert_key,
            result.data());

        this->data_ = result.data();
      }
    };
  }
}

#endif //__VDS_USER_MANAGER_CHANNEL_MESSAGE_TRANSACTION_H_
