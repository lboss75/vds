#include "stdafx.h"
#include "dht_session.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "server.h"
#include "private/server_p.h"
#include "../vds_dht_network/private/dht_network_client_p.h"
#include "messages/dht_route_messages.h"
#include "messages/sync_messages.h"
#include "messages/transaction_log_messages.h"

#define route_client(message_type)\
  case dht::network::message_type_t::message_type: {\
    CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this(), &final_tasks, &result](database_transaction & t) -> expected<void> {\
        binary_deserializer s(message_info.message_data());\
        GET_EXPECTED(message, message_deserialize<dht::messages::message_type>(s));\
        GET_EXPECTED_VALUE(result, (*pthis->sp_->get<dht::network::client>())->apply_message(\
         t,\
         final_tasks,\
         message,\
         message_info));\
       return expected<void>();\
      }));\
      break;\
    }

vds::async_task<vds::expected<bool>> vds::_server::process_message(
  message_info_t message_info) {

  if(this->sp_->get_shutdown_event().is_shuting_down()) {
    co_return false;
  }
  bool result = false;
  std::list<std::function<async_task<expected<void>>()>> final_tasks;
  switch(message_info.message_type()){
  case dht::network::message_type_t::transaction_log_state: {
    CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this(), &final_tasks, &result](database_transaction & t) -> expected<void> {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED(message, message_deserialize<dht::messages::transaction_log_state>(s));
      GET_EXPECTED_VALUE(result, (*pthis->sp_->get<server>())->apply_message(
          t,
          final_tasks,
          message,
          message_info));

      return expected<void>();
    }));
    break;
  }
  case dht::network::message_type_t::transaction_log_request: {
    CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
        [message_info, pthis = this->shared_from_this(), &final_tasks, &result](database_transaction & t) -> expected<void> {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED(message, message_deserialize<dht::messages::transaction_log_request>(s));
      GET_EXPECTED_VALUE(result, (*pthis->sp_->get<server>())->apply_message(
          t,
          final_tasks,
          message,
          message_info));

      return expected<void>();
    }));
    break;
  }
  case dht::network::message_type_t::transaction_log_record: {
    CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this(), &final_tasks, &result](database_transaction & t) -> expected<void> {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED(message, message_deserialize<dht::messages::transaction_log_record>(s));
      GET_EXPECTED_VALUE(result, (*pthis->sp_->get<server>())->apply_message(
          t,
          final_tasks,
          message,
          message_info));
      return expected<void>();
    }));
    break;
  }

    case dht::network::message_type_t::dht_find_node: {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED_ASYNC(message, message_deserialize<dht::messages::dht_find_node>(s));
      GET_EXPECTED_VALUE_ASYNC(result, co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info));
      break;
    }

    case dht::network::message_type_t::dht_find_node_response: {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED_ASYNC(message, message_deserialize<dht::messages::dht_find_node_response>(s));
      GET_EXPECTED_VALUE_ASYNC(result, co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info));
      break;
    }

    case dht::network::message_type_t::dht_ping: {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED_ASYNC(message, message_deserialize<dht::messages::dht_ping>(s));
      GET_EXPECTED_VALUE_ASYNC(result, co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info));
      break;
    }

    case dht::network::message_type_t::dht_pong: {
      binary_deserializer s(message_info.message_data());
      GET_EXPECTED_ASYNC(message, message_deserialize<dht::messages::dht_pong>(s));
      GET_EXPECTED_VALUE_ASYNC(result, co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info));
      break;
    }
 /*   case network::message_type_t::replica_request: {
      this->replica_request_++;

      binary_deserializer s(message_data);
      messages::sync_replica_request message(s);
      auto result = std::make_shared<vds::async_task<vds::expected<void>>>(vds::async_task<vds::expected<void>>::empty());
      *result = (*sp->get<client>())->apply_message(
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
    //  (*sp->get<client>())->apply_message(
    //      sp.create_scope("messages::sync_replica_request"),
    //      this->shared_from_this(),
    //      message)
    //  .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}
    //case network::message_type_t::replica_data: {
    //  binary_deserializer s(message_data);
    //  const messages::sync_replica_data message(s);
    //  (*sp->get<client>())->apply_message(
    //    sp.create_scope("messages::sync_replica_data"),
    //    this->shared_from_this(),
    //    message)
    //    .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}
    //case network::message_type_t::got_replica: {
    //  binary_deserializer s(message_data);
    //  const messages::got_replica message(s);
    //  (*sp->get<client>())->apply_message(
    //    sp.create_scope("messages::sync_replica_data"),
    //    this->shared_from_this(),
    //    message)
    //    .execute([](const std::shared_ptr<std::exception> &) {});
    //  break;
    //}

    //route_client(sync_new_election_request)
    //route_client(sync_new_election_response)

    //route_client(sync_add_message_request)

    //route_client(sync_leader_broadcast_request)
    //route_client(sync_leader_broadcast_response)

    //route_client(sync_replica_operations_request)
    //route_client(sync_replica_operations_response)

    //route_client(sync_looking_storage_request)
    //route_client(sync_looking_storage_response)

    //route_client(sync_snapshot_request)
    //route_client(sync_snapshot_response)

    //route_client(sync_offer_send_replica_operation_request)
    //route_client(sync_offer_remove_replica_operation_request)

    route_client(sync_replica_request)
    route_client(sync_replica_data)
    //
    //route_client(sync_replica_query_operations_request)

    default:{
      co_return vds::make_unexpected<std::runtime_error>("Invalid command");
    }
  }

  while (!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  co_return result;
}



