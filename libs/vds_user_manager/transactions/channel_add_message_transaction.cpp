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
  const vds::asymmetric_private_key &write_key) {

}
