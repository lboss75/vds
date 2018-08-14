/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <include/imessage_map.h>
#include "stdafx.h"
#include "private/dht_session.h"
#include "db_model.h"
#include "include/dht_network_client.h"
#include "private/dht_network_client_p.h"
#include "messages/dht_find_node.h"
#include "messages/dht_find_node_response.h"
#include "messages/transaction_log_request.h"
#include "messages/transaction_log_record.h"
#include "messages/dht_ping.h"
#include "messages/dht_pong.h"
#include "messages/sync_replica_request.h"
#include "messages/transaction_log_state.h"
#include "messages/offer_replica.h"
#include "messages/sync_replica_data.h"
#include "messages/got_replica.h"
#include "messages/sync_new_election.h"
#include "messages/sync_coronation.h"

vds::dht::network::dht_session::dht_session(
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id,
  uint32_t session_id)
: base_class(address, this_node_id, session_id),
  partner_node_id_(partner_node_id) {
}

void vds::dht::network::dht_session::ping_node(
  const service_provider& sp,
  const const_data_buffer & node_id,
  const std::shared_ptr<udp_transport> & transport) {

  vds_assert(node_id != transport->this_node_id());

  this->send_message(
      sp,
      transport,
      (uint8_t)messages::dht_ping::message_id,
      node_id,
      messages::dht_ping(transport->this_node_id()).serialize());
}

vds::session_statistic::session_info vds::dht::network::dht_session::get_statistic() const {
  return session_statistic::session_info{
    this->address().to_string()
  };
}

vds::async_task<> vds::dht::network::dht_session::process_message(
    const vds::service_provider &sp,
    uint8_t message_type,
    const vds::const_data_buffer &message) {

  vds_assert(message.size() >= 32);
  const_data_buffer target_node(message.data(), 32);
  const_data_buffer message_data(message.data() + 32, message.size() - 32);
  if(target_node != this->this_node_id()) {
    return [sp, message_type, target_node, message_data]() {
      (*sp.get<client>())->send_closer(
        sp,
        target_node,
        1,
        static_cast<message_type_t>(message_type),
        message_data);
    };
  }
  else {
    return sp.get<imessage_map>()->process_message(
      sp,
      this->shared_from_this(),
      message_type,
      message_data);
  }
}
