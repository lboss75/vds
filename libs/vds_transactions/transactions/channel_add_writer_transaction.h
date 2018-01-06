#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_control.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "symmetriccrypto.h"

namespace vds {
  namespace transactions {
    class channel_add_writer_transaction {
    public:
      static const uint8_t message_id = 'w';

      channel_add_writer_transaction(
          const guid & channel_id,
          const certificate & owner_cert,
          const certificate & read_cert,
          const asymmetric_private_key & read_private_key,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key)
      : channel_id_(channel_id),
        owner_cert_id_(cert_control::get_id(owner_cert))
      {
        binary_serializer s;
        s << read_cert.der() << read_private_key.der(std::string())
          << write_cert.der() << write_private_key.der(std::string());

        auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

        binary_serializer crypted;
        crypted
            << owner_cert.public_key().encrypt(key.serialize())
            << symmetric_encrypt::encrypt(key, s.data());

        this->data_ = crypted.data();
      }

      void serialize(binary_serializer & s) const {
        s << this->data_;
      }

    private:
      guid channel_id_;
      guid owner_cert_id_;
      const_data_buffer data_;
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
