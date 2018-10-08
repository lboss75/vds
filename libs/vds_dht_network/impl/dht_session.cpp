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
  const service_provider * sp,
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id,
  const const_data_buffer& session_key)
  : base_class(
      sp,
      address,
      this_node_id,
      partner_node_id,
      session_key) {
}

std::future<void> vds::dht::network::dht_session::ping_node(
  
  const const_data_buffer& node_id,
  const std::shared_ptr<iudp_transport>& transport) {

  //vds_assert(node_id != transport->this_node_id());

  co_await this->send_message(
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

std::future<void> vds::dht::network::dht_session::process_message(
  
  const std::shared_ptr<iudp_transport>& transport,
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
  this->sp_->get<logger>()->trace(
    "dht_session",
    "receive %d from %s to %s",
    message_type,
    base64::from_bytes(source_node).c_str(),
    base64::from_bytes(target_node).c_str());

  (*this->sp_->get<client>())->add_route(source_node, hops, this->shared_from_this());

  if (target_node != this->this_node_id()) {
    if (hops == std::numeric_limits<uint16_t>::max()) {
      co_return;
    }

    this->sp_->get<logger>()->trace(
      "dht_session",
      "redirect %d from %s to %s",
      message_type,
      base64::from_bytes(source_node).c_str(),
      base64::from_bytes(target_node).c_str());

    co_await (*this->sp_->get<client>())->proxy_message(
      target_node,
      (message_type_t)message_type,
      message,
      source_node,
      hops + 1);
  }
  else {
    co_await this->sp_->get<imessage_map>()->process_message(
      imessage_map::message_info_t{
        this->shared_from_this(),
        static_cast<message_type_t>(message_type),
        message,
        source_node,
        hops
    });
  }
}
