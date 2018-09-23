#include "stdafx.h"
#include "../vds_dht_network/private/dht_session.h"
#include "messages/transaction_log_state.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "messages/transaction_log_request.h"
#include "messages/transaction_log_record.h"
#include "messages/dht_find_node.h"
#include "messages/dht_find_node_response.h"
#include "messages/dht_ping.h"
#include "messages/dht_pong.h"
#include "messages/sync_replica_request.h"
#include "messages/offer_replica.h"
#include "messages/sync_replica_data.h"
#include "server.h"
#include "private/server_p.h"
#include "../vds_dht_network/private/dht_network_client_p.h"
#include "messages/got_replica.h"
#include "messages/sync_new_election.h"
#include "messages/sync_add_message.h"
#include "messages/sync_leader_broadcast.h"
#include "messages/sync_replica_operations.h"
#include "messages/sync_looking_storage.h"
#include "messages/sync_snapshot.h"
#include "messages/sync_offer_send_replica_operation.h"
#include "messages/sync_offer_remove_replica_operation.h"
#include "messages/sync_replica_query_operations.h"

#define route_client(message_type)\
  case dht::network::message_type_t::message_type: {\
      return sp.get<db_model>()->async_transaction(sp, [sp, message_info](database_transaction & t) {\
        binary_deserializer s(message_info.message_data());\
        dht::messages::message_type message(s);\
        (*sp.get<dht::network::client>())->apply_message(\
        sp.create_scope("messages::" #message_type),\
         t,\
         message,\
         message_info);\
        return true;\
      });\
      break;\
    }

vds::async_task<void> vds::_server::process_message(
  const service_provider& scope,
  const message_info_t & message_info) {

  if(scope.get_shutdown_event().is_shuting_down()) {
    return vds::async_task<void>::empty();
  }

  auto sp = scope.create_scope(__FUNCTION__);
  switch(message_info.message_type()){
  case dht::network::message_type_t::transaction_log_state: {
    auto result = std::make_shared<vds::async_task<void>>(vds::async_task<void>::empty());
    return sp.get<db_model>()->async_transaction(sp, [sp, message_info, result](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      dht::messages::transaction_log_state message(s);
      *result = (*sp.get<server>())->apply_message(
          sp.create_scope("messages::transaction_log_state"),
          t,
          message,
          message_info);
      return true;
    }).then([sp, result]() {
      mt_service::async(sp, [result]() {
        result->execute([](const std::shared_ptr<std::exception> & ) {
        });
      });
    });
    break;
  }
  case dht::network::message_type_t::transaction_log_request: {
    return sp.get<db_model>()->async_transaction(
        sp,
        [sp, message_info](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      dht::messages::transaction_log_request message(s);
      (*sp.get<server>())->apply_message(
          sp.create_scope("messages::transaction_log_request"),
          t,
          message,
          message_info);
      return true;
    });
    break;
  }
  case dht::network::message_type_t::transaction_log_record: {
    return sp.get<db_model>()->async_transaction(sp, [sp, message_info](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      dht::messages::transaction_log_record message(s);
      (*sp.get<server>())->apply_message(
          sp.create_scope("messages::transaction_log_record"),
          t,
          message,
          message_info);
      return true;
    });
    break;
  }

    case dht::network::message_type_t::dht_find_node: {
      binary_deserializer s(message_info.message_data());
      dht::messages::dht_find_node message(s);
      (*sp.get<dht::network::client>())->apply_message(
          sp.create_scope("messages::dht_find_node"),
          message,
          message_info);
      break;
    }

    case dht::network::message_type_t::dht_find_node_response: {
      binary_deserializer s(message_info.message_data());
      dht::messages::dht_find_node_response message(s);
      auto result = std::make_shared<vds::async_task<void>>(vds::async_task<void>::empty());
      *result = (*sp.get<dht::network::client>())->apply_message(
          sp.create_scope("messages::dht_find_node_response"),
          message,
          message_info);
      mt_service::async(sp, [result]() mutable {
        result->execute([](const std::shared_ptr<std::exception> & ) {});
      });
      break;
    }

    case dht::network::message_type_t::dht_ping: {
      binary_deserializer s(message_info.message_data());
      dht::messages::dht_ping message(s);
      (*sp.get<dht::network::client>())->apply_message(
          sp.create_scope("messages::dht_ping"),
          message,
          message_info);
      break;
    }

    case dht::network::message_type_t::dht_pong: {
      binary_deserializer s(message_info.message_data());
      dht::messages::dht_pong message(s);
      (*sp.get<dht::network::client>())->apply_message(
          sp.create_scope("messages::dht_pong"),
          message,
          message_info);
      break;
    }
 /*   case network::message_type_t::replica_request: {
      this->replica_request_++;

      binary_deserializer s(message_data);
      messages::sync_replica_request message(s);
      auto result = std::make_shared<vds::async_task<void>>(vds::async_task<void>::empty());
      *result = (*sp.get<client>())->apply_message(
        sp.create_scope("messages::sync_replica_request"),
        this->shared_from_this(),
        message);
      mt_service::async(sp, [result]() mutable {
        result->execute([](const std::shared_ptr<std::exception> &) {});
      });
      break;
    }
 */   //case network::message_type_t::offer_replica: {
    //  this->offer_replica_++;

    //  binary_deserializer s(message_data);
    //  messages::offer_replica message(s);
    //  (*sp.get<client>())->apply_message(
    //      sp.create_scope("messages::sync_replica_request"),
    //      this->shared_from_this(),
    //      message)
    //  .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}
    //case network::message_type_t::replica_data: {
    //  binary_deserializer s(message_data);
    //  const messages::sync_replica_data message(s);
    //  (*sp.get<client>())->apply_message(
    //    sp.create_scope("messages::sync_replica_data"),
    //    this->shared_from_this(),
    //    message)
    //    .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}
    //case network::message_type_t::got_replica: {
    //  binary_deserializer s(message_data);
    //  const messages::got_replica message(s);
    //  (*sp.get<client>())->apply_message(
    //    sp.create_scope("messages::sync_replica_data"),
    //    this->shared_from_this(),
    //    message)
    //    .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}

    route_client(sync_new_election_request)
    route_client(sync_new_election_response)

    route_client(sync_add_message_request)

    route_client(sync_leader_broadcast_request)
    route_client(sync_leader_broadcast_response)

    route_client(sync_replica_operations_request)
    route_client(sync_replica_operations_response)

    route_client(sync_looking_storage_request)
    route_client(sync_looking_storage_response)

    route_client(sync_snapshot_request)
    route_client(sync_snapshot_response)

    route_client(sync_offer_send_replica_operation_request)
    route_client(sync_offer_remove_replica_operation_request)

    route_client(sync_replica_request)
    route_client(sync_replica_data)
    
    route_client(sync_replica_query_operations_request)

    default:{
      throw std::runtime_error("Invalid command");
    }
  }
  return vds::async_task<void>::empty();
}



