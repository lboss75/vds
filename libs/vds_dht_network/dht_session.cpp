//
// Created by vadim on 15.03.18.
//

#include "dht_session.h"

vds::dht::network::dht_session::dht_session(
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id)
: base_class(address, this_node_id) {
}

vds::async_task<> vds::dht::network::dht_session::process_message(
  const service_provider& sp,
  uint8_t message_type,
  const const_data_buffer& message) {

  return async_task<>::empty();
}
