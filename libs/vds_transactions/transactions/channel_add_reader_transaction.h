#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "channel_message_transaction.h"

namespace vds {
  namespace transactions {
    class channel_add_reader_transaction : public channel_message_transaction {
    public:
      channel_add_reader_transaction(
          const guid & target_channel_id,
          const certificate & target_cert,
          const certificate & sign_cert,
          const asymmetric_private_key & sing_cert_private_key,

          const certificate & read_cert,
          const asymmetric_private_key & read_private_key)
          : channel_message_transaction(
          channel_message_id::channel_add_reader_transaction,
		  target_channel_id,
          target_cert,
		  cert_control::get_id(sign_cert),
          sing_cert_private_key,
          (
              binary_serializer()
                  << read_cert.der()
                  << read_private_key.der(std::string())
          ).data()) {

		  if (!asymmetric_sign_verify::verify(
			  hash::sha256(),
			  sign_cert.public_key(),
			  this->signature_,
			  (binary_serializer()
				  << (uint8_t)this->message_id_
				  << this->channel_id_
				  << this->read_cert_id_
				  << this->write_cert_id_
				  << this->data_).data())) {
			  throw std::runtime_error("Write signature error");
		  }

      }

      channel_add_reader_transaction(binary_deserializer & s)
      : channel_message_transaction(s){
      }
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
