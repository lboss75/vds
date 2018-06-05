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
#include "messages/replica_request.h"
#include "messages/transaction_log_state.h"
#include "messages/offer_replica.h"

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

vds::async_task<> vds::dht::network::dht_session::process_message(
  const service_provider& scope,
  uint8_t message_type,
  const const_data_buffer& message_data) {
  auto sp = scope.create_scope(__FUNCTION__);
  switch((network::message_type_t)message_type){
  case network::message_type_t::transaction_log_state: {
    this->transaction_log_state_count_++;

    auto result = std::make_shared<async_task<>>(async_task<>::empty());
    return sp.get<db_model>()->async_transaction(sp, [sp, message_data, result](database_transaction & t) {
      binary_deserializer s(message_data);
      messages::transaction_log_state message(s);
      *result = (*sp.get<client>())->apply_message(sp.create_scope("messages::transaction_log_state"), t, message);
      return true;
    }).then([sp, result]() {
      mt_service::async(sp, [result]() {
        result->execute([](const std::shared_ptr<std::exception> & ) {
        });
      });
    });
    break;
  }
  case network::message_type_t::transaction_log_request: {
    this->transaction_log_request_++;

    auto result = std::make_shared<async_task<>>(async_task<>::empty());
    return sp.get<db_model>()->async_transaction(sp, [sp, message_data, result](database_transaction & t) {
      binary_deserializer s(message_data);
      messages::transaction_log_request message(s);
      *result = (*sp.get<client>())->apply_message(sp.create_scope("messages::transaction_log_request"), t, message);
      return true;
    }).then([sp, result]() {
      mt_service::async(sp, [result]() {
        result->execute([](const std::shared_ptr<std::exception> & ) {
        });
      });
    });
    break;
  }
  case network::message_type_t::transaction_log_record: {
    this->transaction_log_record_++;

    return sp.get<db_model>()->async_transaction(sp, [sp, message_data](database_transaction & t) {
      binary_deserializer s(message_data);
      messages::transaction_log_record message(s);
      (*sp.get<client>())->apply_message(sp.create_scope("messages::transaction_log_record"), t, message);
      return true;
    });
    break;
  }

    case network::message_type_t::dht_find_node: {
      this->dht_find_node_++;

      binary_deserializer s(message_data);
      messages::dht_find_node message(s);
      (*sp.get<client>())->apply_message(sp.create_scope("messages::dht_find_node"), message);
      break;
    }

    case network::message_type_t::dht_find_node_response: {
      this->dht_find_node_response_++;

      binary_deserializer s(message_data);
      messages::dht_find_node_response message(s);
      auto result = std::make_shared<async_task<>>(async_task<>::empty());
      *result = (*sp.get<client>())->apply_message(sp.create_scope("messages::dht_find_node_response"), this->shared_from_this(), message);
      mt_service::async(sp, [result]() mutable {
        result->execute([](const std::shared_ptr<std::exception> & ) {});
      });
      break;
    }

    case network::message_type_t::dht_ping: {
      this->dht_ping_++;

      binary_deserializer s(message_data);
      messages::dht_ping message(s);
      (*sp.get<client>())->apply_message(sp.create_scope("messages::dht_ping"), this->shared_from_this(), message);
      break;
    }

    case network::message_type_t::dht_pong: {
      this->dht_pong_++;

      binary_deserializer s(message_data);
      messages::dht_pong message(s);
      (*sp.get<client>())->apply_message(sp.create_scope("messages::dht_pong"), this->shared_from_this(), message);
      break;
    }
    case network::message_type_t::replica_request: {
      this->replica_request_++;

      binary_deserializer s(message_data);
      messages::replica_request message(s);
      auto result = std::make_shared<async_task<>>(async_task<>::empty());
      *result = (*sp.get<client>())->apply_message(
        sp.create_scope("messages::replica_request"),
        this->shared_from_this(),
        message);
      mt_service::async(sp, [result]() mutable {
        result->execute([](const std::shared_ptr<std::exception> &) {});
      });
      break;
    }
    case network::message_type_t::offer_replica: {
      this->offer_replica_++;

      binary_deserializer s(message_data);
      messages::offer_replica message(s);
      auto result = std::make_shared<async_task<>>(async_task<>::empty());
      *result = (*sp.get<client>())->apply_message(
          sp.create_scope("messages::replica_request"),
          this->shared_from_this(),
          message);
      mt_service::async(sp, [result]() mutable {
        result->execute([](const std::shared_ptr<std::exception> &) {});
      });
      break;
    }
    default:{
      throw std::runtime_error("Invalid command");
    }
  }
  return async_task<>::empty();
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
