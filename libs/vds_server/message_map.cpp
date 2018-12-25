#include "stdafx.h"
#include "../vds_dht_network/private/dht_session.h"
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
      co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this()](database_transaction & t) {\
        binary_deserializer s(message_info.message_data());\
        auto message = message_deserialize<dht::messages::message_type>(s);\
        (*pthis->sp_->get<dht::network::client>())->apply_message(\
         t,\
         message,\
         message_info).get();\
      });\
      break;\
    }

vds::async_task<void> vds::_server::process_message(
  
  const message_info_t & message_info) {

  if(this->sp_->get_shutdown_event().is_shuting_down()) {
    co_return;
  }

  switch(message_info.message_type()){
  case dht::network::message_type_t::transaction_log_state: {
    co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this()](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::transaction_log_state>(s);
      (*pthis->sp_->get<server>())->apply_message(
          t,
          message,
          message_info).get();
    });
    break;
  }
  case dht::network::message_type_t::transaction_log_request: {
    co_await this->sp_->get<db_model>()->async_transaction(
        [message_info, pthis = this->shared_from_this()](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::transaction_log_request>(s);
      (*pthis->sp_->get<server>())->apply_message(
          t,
          message,
          message_info).get();
    });
    break;
  }
  case dht::network::message_type_t::transaction_log_record: {
    co_await this->sp_->get<db_model>()->async_transaction([message_info, pthis = this->shared_from_this()](database_transaction & t) {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::transaction_log_record>(s);
      (*pthis->sp_->get<server>())->apply_message(
          t,
          message,
          message_info).get();
      return true;
    });
    break;
  }

    case dht::network::message_type_t::dht_find_node: {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::dht_find_node>(s);
      co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info);
      break;
    }

    case dht::network::message_type_t::dht_find_node_response: {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::dht_find_node_response>(s);
      co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info);
      break;
    }

    case dht::network::message_type_t::dht_ping: {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::dht_ping>(s);
      co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info);
      break;
    }

    case dht::network::message_type_t::dht_pong: {
      binary_deserializer s(message_info.message_data());
      auto message = message_deserialize<dht::messages::dht_pong>(s);
      co_await (*this->sp_->get<dht::network::client>())->apply_message(
          message,
          message_info);
      break;
    }
 /*   case network::message_type_t::replica_request: {
      this->replica_request_++;

      binary_deserializer s(message_data);
      messages::sync_replica_request message(s);
      auto result = std::make_shared<vds::async_task<void>>(vds::async_task<void>::empty());
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
}



