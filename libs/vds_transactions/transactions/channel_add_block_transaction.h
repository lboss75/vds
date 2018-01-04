#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_BLOCK_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_BLOCK_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <cert_control.h>
#include "const_data_buffer.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "symmetriccrypto.h"
#include "guid.h"

namespace vds {
  namespace transactions {
    class channel_add_block_transaction {
    public:
      static const uint8_t category_id = transaction_log::transactions_category_id;
      static const uint8_t message_id = 'b';

      channel_add_block_transaction(
          const certificate & cert,
          const asymmetric_private_key & cert_key,
          const const_data_buffer & block_id,
          const const_data_buffer & key)
      : cert_id_(cert_control::get_id(cert)),
        block_id_(block_id){

        binary_serializer result;
        result << cert.public_key().encrypt(key);

        result << asymmetric_sign::signature(
            hash::sha256(),
            cert_key,
            result.data());

        this->data_ = result.data();
      }

    private:
      guid cert_id_;
      const_data_buffer block_id_;
      const_data_buffer data_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_BLOCK_TRANSACTION_H_
