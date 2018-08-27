/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
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
#include "imessage_map.h"

vds::dht::network::dht_session::dht_session(
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id,
  const const_data_buffer& session_key)
  : base_class(
      address,
      this_node_id,
      partner_node_id,
      session_key) {
}

void vds::dht::network::dht_session::ping_node(
  const service_provider& sp,
  const const_data_buffer& node_id,
  const std::shared_ptr<udp_transport>& transport) {

  vds_assert(node_id != transport->this_node_id());

  this->send_message(
    sp,
    transport,
    (uint8_t)messages::dht_ping::message_id,
    node_id,
    messages::dht_ping().serialize());
}

vds::session_statistic::session_info vds::dht::network::dht_session::get_statistic() const {
  return session_statistic::session_info{
    this->address().to_string()
  };
}

vds::async_task<> vds::dht::network::dht_session::process_message(
  const service_provider& sp,
  const std::shared_ptr<udp_transport>& transport,
  uint8_t message_type,
  const const_data_buffer & target_node,
  const const_data_buffer & source_node,
  uint16_t hops,
  const const_data_buffer& message) {

  //if (message_type == (uint8_t)message_type_t::sync_looking_storage_response
  //  || message_type == (uint8_t)message_type_t::sync_looking_storage_request) {
  //  std::cout
  //    << base64::from_bytes(source_node)
  //    << "=>"
  //    << base64::from_bytes(target_node)
  //    << ": "
  //    << std::to_string((message_type_t)message_type)
  //    << "\n";
  //}
    sp.get<logger>()->trace(
    "dht_session",
    sp,
    "receive %d from %s to %s",
    message_type,
    base64::from_bytes(source_node).c_str(),
    base64::from_bytes(target_node).c_str());

  (*sp.get<client>())->add_route(sp, source_node, hops, this->shared_from_this());

  if (target_node != this->this_node_id()) {
    if (hops == std::numeric_limits<uint16_t>::max()) {
      return async_task<>::empty();
    }

    return [sp, message_type, target_node, message, source_node, hops]() {
      sp.get<logger>()->trace(
        "dht_session",
        sp,
        "redirect %d from %s to %s",
        message_type,
        base64::from_bytes(source_node).c_str(),
        base64::from_bytes(target_node).c_str());

      (*sp.get<client>())->proxy_message(
        sp,
        target_node,
        (message_type_t)message_type,
        message,
        source_node,
        hops + 1);
    };
  }
  return sp.get<imessage_map>()->process_message(
    sp,
    imessage_map::message_info_t{
      this->shared_from_this(),
      static_cast<message_type_t>(message_type),
      message,
      source_node,
      hops
    });
}
