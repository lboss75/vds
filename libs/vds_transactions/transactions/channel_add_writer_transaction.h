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
#include "channel_message_transaction.h"

namespace vds {
  namespace transactions {
    class channel_add_writer_transaction : public channel_message_transaction {
    public:
      channel_add_writer_transaction(
          const guid & channel_id,
          const certificate & target_cert,
          const guid & sing_cert_id,
          const asymmetric_private_key & sing_cert_private_key,
		  const guid & target_channel_id,
		  const std::string & name,
          const certificate & read_cert,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key)
      : channel_message_transaction(
          channel_message_id::channel_add_writer_transaction,
          channel_id,
          target_cert,
          sing_cert_id,
          sing_cert_private_key,
          (
            binary_serializer()
			    << target_channel_id
			    << name
                << read_cert.der()
                << write_cert.der()
                << write_private_key.der(std::string())
          ).data()) {
      }

      channel_add_writer_transaction(binary_deserializer & s)
          : channel_message_transaction(s){
      }

	  template <typename target>
	  static void parse_message(binary_deserializer &data_stream, target t) {

		  std::string name;
		  guid target_channel_id;
		  certificate read_cert;
		  certificate write_cert;
		  const_data_buffer write_private_key_der;

		  data_stream >> target_channel_id  >> name >> read_cert >> write_cert >> write_private_key_der;

		  t(
			  target_channel_id,
			  name,
			read_cert,
			write_cert,
			asymmetric_private_key::parse_der(write_private_key_der, std::string()));

	  }
	  /*
      static void apply_message(
          const service_provider &sp,
          database_transaction &t,
          binary_deserializer &data_stream,
		  const guid & channel_id,
		  const certificate &device_cert) {

		  parse_message(data_stream, [sp, &t, channel_id, device_cert](
			  const std::string & channel_name,
			  const certificate & read_cert,
			  const certificate & write_cert,
			  const asymmetric_private_key & write_private_key) {
			  dbo::channel t1;
			  auto st = t.get_reader(t1.select(t1.id).where(t1.id == channel_id));
			  if (!st.execute()) {
				  t.execute(t1.insert(
					  t1.id = channel_id,
					  t1.channel_type = (uint8_t)dbo::channel::channel_type_t::simple,
					  t1.name = channel_name,
					  t1.read_cert = cert_control::get_id(read_cert),
					  t1.write_cert = cert_control::get_id(write_cert)
				  ));
			  }
			  else {
				  t.execute(t1.update(
					  t1.name = channel_name,
					  t1.read_cert = cert_control::get_id(read_cert),
					  t1.write_cert = cert_control::get_id(write_cert)
				  ).where(t1.id == channel_id));
			  }

			  sp.get<logger>()->info("TRLOG", sp, "Read certificate %s for channel %s (%s)",
				  cert_control::get_id(read_cert).str().c_str(),
				  channel_id.str().c_str(),
				  channel_name.c_str());

			  cert_control::register_certificate(t, read_cert);

			  sp.get<logger>()->info("TRLOG", sp, "Write certificate %s for channel %s (%s)",
				  cert_control::get_id(write_cert).str().c_str(),
				  channel_id.str().c_str(),
				  channel_name.c_str());

			  cert_control::register_certificate(t, write_cert);

			  dbo::certificate_private_key t2;
			  t.execute(t2.insert(
				  t2.id = cert_control::get_id(write_cert),
				  t2.owner_id = channel_id,
				  t2.body = device_cert.public_key().encrypt(write_private_key.der(std::string()))
			  ));
		  });
      }
	  */
    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_WRITER_TRANSACTION_H_
