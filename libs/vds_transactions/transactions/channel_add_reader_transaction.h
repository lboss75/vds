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
          const guid & channel_id,
          const certificate & target_cert,
          const certificate & sign_cert,
          const asymmetric_private_key & sing_cert_private_key,

		  const guid & target_channel_id,
		  const std::string & name,
		  const certificate & read_cert,
		  const asymmetric_private_key & read_private_key,
		  const certificate & write_cert)
          : channel_message_transaction(
          channel_message_id::channel_add_reader_transaction,
		  channel_id,
          target_cert,
		  cert_control::get_id(sign_cert),
          sing_cert_private_key,
          (
              binary_serializer()
			  << target_channel_id
			  << name
			  << read_cert.der()
              << read_private_key.der(std::string())
			  << write_cert.der()
          ).data()) {

      }

      channel_add_reader_transaction(binary_deserializer & s)
      : channel_message_transaction(s){
      }

	  template <typename target>
	  static void parse_message(binary_deserializer &data_stream, target t) {

		  std::string name;
		  guid target_channel_id;
		  certificate read_cert;
		  const_data_buffer read_private_key_der;
		  certificate write_cert;

		  data_stream
			  >> target_channel_id
			  >> name
			  >> read_cert
			  >> read_private_key_der
			  >> write_cert;

		  t(
			  target_channel_id,
			  name,
			  read_cert,
			  asymmetric_private_key::parse_der(read_private_key_der, std::string()),
			  write_cert);

	  }

    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
