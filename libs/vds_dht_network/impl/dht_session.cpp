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

vds::dht::network::dht_session::dht_session(
  const network_address& address,
  const const_data_buffer& this_node_id,
  const const_data_buffer& partner_node_id)
: base_class(address, this_node_id),
  partner_node_id_(partner_node_id),
  offer_replica_(0),
  replica_request_(0),
  dht_pong_(0),
  dht_ping_(0),
  dht_find_node_response_(0),
  dht_find_node_(0),
  transaction_log_record_(0),
  transaction_log_request_(0),
  transaction_log_state_count_(0) {
}

void vds::dht::network::dht_session::ping_node(
  const service_provider& sp,
  const const_data_buffer & node_id,
  const std::shared_ptr<udp_transport> & transport) {

  this->send_message(
      sp,
      transport,
      (uint8_t)messages::dht_ping::message_id,
      messages::dht_ping(node_id, transport->this_node_id()).serialize());
}

vds::session_statistic::session_info vds::dht::network::dht_session::get_statistic() const {
  return session_statistic::session_info{
    this->address().to_string(),
    this->offer_replica_,
    this->replica_request_,
    this->dht_pong_,
    this->dht_ping_,
    this->dht_find_node_response_,
    this->dht_find_node_,
    this->transaction_log_record_,
    this->transaction_log_request_,
    this->transaction_log_state_count_
  };
}
