//
// Created by vadim on 19.11.17.
//

#include "channel_add_message_transaction.h"

vds::channel_add_message_transaction::create_channel::create_channel(
  const vds::guid &channel_id,
  const vds::guid &read_cert_id,
  const vds::certificate &read_cert,
  const vds::asymmetric_private_key &read_cert_key,
  const vds::guid &write_cert_id,
  const vds::certificate &write_cert,
  const vds::asymmetric_private_key &write_key)
: channel_id_(channel_id),
	read_cert_id_(read_cert_id),
	read_cert_(read_cert),
	read_cert_key_(read_cert_key),
	write_cert_id_(write_cert_id),
	write_cert_(write_cert),
	write_key_(write_key)
{

}
