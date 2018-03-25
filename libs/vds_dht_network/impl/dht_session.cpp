/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_session.h"
#include "messages/offer_move_replica.h"
#include "db_model.h"
#include "include/dht_network_client.h"
#include "private/dht_network_client_p.h"

vds::dht::network::dht_session::dht_session(
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id)
: base_class(address, this_node_id) {
}

vds::async_task<> vds::dht::network::dht_session::process_message(
  const service_provider& sp,
  uint8_t message_type,
  const const_data_buffer& message_data) {

  switch((network::message_type_t)message_type){
    case network::message_type_t::offer_move_replica: {
      return sp.get<db_model>()->async_transaction(sp, [sp, message_data](database_transaction & t){
        binary_deserializer s(message_data);
        messages::offer_move_replica message(s);
        (*sp.get<client>())->apply_message(sp, t, message);
        return true;
      });
      break;
    }
    default:{
      throw std::runtime_error("Invalid command");
    }
  }
  return async_task<>::empty();
}
