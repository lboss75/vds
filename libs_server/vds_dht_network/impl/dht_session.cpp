/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_session.h"
#include "db_model.h"
#include "include/dht_network_client.h"
#include "dht_network_client_p.h"
#include "messages/dht_route_messages.h"
#include "imessage_map.h"

vds::dht::network::dht_session::dht_session(
  const service_provider * sp,
  const network_address& address,
  const const_data_buffer& this_node_id,
  asymmetric_public_key partner_node_key,
  const const_data_buffer& partner_node_id,
  const const_data_buffer& session_key) noexcept
  : base_class(
      sp,
      address,
      this_node_id,
      std::move(partner_node_key),
      partner_node_id,
      session_key) {
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_session::ping_node(
  const const_data_buffer& node_id,
  const std::shared_ptr<iudp_transport>& transport) {

  GET_EXPECTED(message,
    message_create<messages::dht_ping>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()));
  return this->send_message(
    transport,
    (uint8_t)messages::dht_ping::message_id,
    node_id,
    message_serialize(message));
}

vds::session_statistic::session_info vds::dht::network::dht_session::get_statistic() const {
  std::unique_lock<std::mutex> send_lock(const_cast<dht_session *>(this)->metrics_mutex_);

  return session_statistic::session_info{
    base64::from_bytes(this->partner_node_id()),
    this->address().to_string(),
    false,
    false,
    this->metrics_
  };
}

vds::async_task<vds::expected<bool>> vds::dht::network::dht_session::process_message(
  const std::shared_ptr<iudp_transport>& transport,
  uint8_t message_type,
  const const_data_buffer & target_node,
  const std::vector<const_data_buffer> & hops,
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
    base64::from_bytes(hops[0]).c_str(),
    base64::from_bytes(target_node).c_str());

  (*this->sp_->get<client>())->add_route(hops, std::static_pointer_cast<dht_session>(this->shared_from_this()));

  if (target_node != this->this_node_id()) {
    this->sp_->get<logger>()->trace(
      "dht_session",
      "redirect %d from %s to %s",
      message_type,
      base64::from_bytes(hops[0]).c_str(),
      base64::from_bytes(target_node).c_str());

    CHECK_EXPECTED_ASYNC(co_await (*this->sp_->get<client>())->proxy_message(
      target_node,
      (message_type_t)message_type,
      message,
      hops));
    co_return true;
  }
  else {
    co_return co_await this->sp_->get<imessage_map>()->process_message(
      imessage_map::message_info_t(
        std::static_pointer_cast<dht_session>(this->shared_from_this()),
        static_cast<message_type_t>(message_type),
        message,
        hops
    ));
  }
}
