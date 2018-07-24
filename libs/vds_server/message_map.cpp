#include "stdafx.h"
#include "../vds_dht_network/private/dht_session.h"
#include "../vds_dht_network/messages/transaction_log_state.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "../vds_dht_network/messages/transaction_log_request.h"
#include "../vds_dht_network/messages/transaction_log_record.h"
#include "../vds_dht_network/messages/dht_find_node.h"
#include "../vds_dht_network/messages/dht_find_node_response.h"
#include "../vds_dht_network/messages/dht_ping.h"
#include "../vds_dht_network/messages/dht_pong.h"
#include "../vds_dht_network/messages/object_request.h"
#include "../vds_dht_network/messages/offer_replica.h"
#include "../vds_dht_network/messages/replica_data.h"
#include "server.h"
#include "private/server_p.h"

vds::async_task<> vds::dht::network::dht_session::process_message(
  const service_provider& scope,
  uint8_t message_type,
  const const_data_buffer& message_data) {

  if(scope.get_shutdown_event().is_shuting_down()) {
    return async_task<>::empty();
  }

  auto sp = scope.create_scope(__FUNCTION__);
  switch((network::message_type_t)message_type){
  case network::message_type_t::transaction_log_state: {
    this->transaction_log_state_count_++;

    auto result = std::make_shared<async_task<>>(async_task<>::empty());
    return sp.get<db_model>()->async_transaction(sp, [sp, message_data, result](database_transaction & t) {
      binary_deserializer s(message_data);
      messages::transaction_log_state message(s);
      *result = (*sp.get<server>())->apply_message(sp.create_scope("messages::transaction_log_state"), t, message);
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

    return sp.get<db_model>()->async_transaction(sp, [sp, message_data](database_transaction & t) {
      binary_deserializer s(message_data);
      messages::transaction_log_request message(s);
      (*sp.get<client>())->apply_message(sp.create_scope("messages::transaction_log_request"), t, message);
      return true;
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
      messages::object_request message(s);
      auto result = std::make_shared<async_task<>>(async_task<>::empty());
      *result = (*sp.get<client>())->apply_message(
        sp.create_scope("messages::object_request"),
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
      (*sp.get<client>())->apply_message(
          sp.create_scope("messages::object_request"),
          this->shared_from_this(),
          message)
      .execute([](const std::shared_ptr<std::exception> &) {});
      break;
    }
    case network::message_type_t::replica_data: {
      binary_deserializer s(message_data);
      messages::replica_data message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::replica_data"),
        this->shared_from_this(),
        message)
        .execute([](const std::shared_ptr<std::exception> &) {});
      break;
    }
    case network::message_type_t::got_replica: {
      binary_deserializer s(message_data);
      messages::got_replica message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::replica_data"),
        this->shared_from_this(),
        message)
        .execute([](const std::shared_ptr<std::exception> &) {});
      break;
    }

    case network::message_type_t::sync_new_election_request: {
      binary_deserializer s(message_data);
      const messages::sync_new_election_request message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::sync_new_election_request"),
        message);
      break;
    }

    case network::message_type_t::sync_new_election_response: {
      binary_deserializer s(message_data);
      const messages::sync_new_election_response message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::sync_new_election_response"),
        message);
      break;
    }

    case network::message_type_t::sync_coronation_request: {
      binary_deserializer s(message_data);
      const messages::sync_coronation_request message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::sync_coronation_request"),
        message);
      break;
    }

    case network::message_type_t::sync_coronation_response: {
      binary_deserializer s(message_data);
      const messages::sync_coronation_response message(s);
      (*sp.get<client>())->apply_message(
        sp.create_scope("messages::sync_coronation_response"),
        message);
      break;
    }

    default:{
      throw std::runtime_error("Invalid command");
    }
  }
  return async_task<>::empty();
}


