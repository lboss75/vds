#include "stdafx.h"
#include "test_sync_process.h"
#include "db_model.h"
#include "asymmetriccrypto.h"
#include "udp_socket.h"
#include "../../libs/vds_dht_network/private/dht_session.h"
#include "../../libs/vds_dht_network/private/dht_network_client_p.h"
#include "messages/sync_new_election.h"
#include "messages/sync_add_message.h"
#include "messages/sync_leader_broadcast.h"
#include "messages/sync_replica_operations.h"
#include "messages/sync_looking_storage.h"
#include "messages/sync_snapshot.h"
#include "messages/sync_offer_send_replica_operation.h"
#include "messages/sync_offer_remove_replica_operation.h"
#include "messages/sync_replica_query_operations.h"
#include "messages/sync_replica_request.h"
#include "messages/sync_replica_data.h"

#define SERVER_COUNT 10

TEST(test_vds_dht_network, test_sync_process) {
#ifdef _WIN32
  //Initialize Winsock
  WSADATA wsaData;
  if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
    auto error = WSAGetLastError();
    throw std::system_error(error, std::system_category(), "Initiates Winsock");
  }
#endif

  std::vector<std::shared_ptr<test_server>> servers;

  auto hab = std::make_shared<transport_hab>();

  for(int i = 0; i < SERVER_COUNT; ++i) {
    auto server = std::make_shared<test_server>(
      vds::network_address(AF_INET, "localhost", i), hab);
    server->start(hab, i);
    servers.push_back(server);
    hab->add(server->address(), server.get());

    for(int j = 0; j < i; ++j) {
      hab->attach(servers[j], server);
    }
  }

  uint8_t object_id[32] = {
    0xC9, 0x9F, 0x11, 0x85,
    0xFE, 0x2E, 0x42, 0x7F,
    0xA7, 0x08, 0x11, 0xD2,
    0xDF, 0xC8, 0x91, 0xF6
  };

  servers[4]->add_sync_entry(vds::const_data_buffer(object_id, sizeof(object_id)), 0x100);

  for(auto server : servers){
    server->stop();
  }
}

vds::async_task<> transport_hab::write_async(
  const vds::service_provider& sp,
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer & source_node_id) {
  auto p = this->servers_.find(datagram.address());
  if(this->servers_.end() == p) {
    auto a = datagram.address().to_string();
    return vds::async_task<>(std::make_shared<std::runtime_error>("Invalid address " + a));
  }
  else {
    return p->second->process_datagram(sp, datagram, source_node_id);
  }
}

void transport_hab::add(const vds::network_address& address, test_server* server) {
  this->servers_[address] = server;
}

void transport_hab::attach(const std::shared_ptr<test_server>& server1, const std::shared_ptr<test_server>& server2) {
  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  auto session = std::make_shared<vds::dht::network::dht_session>(
    server1->address(),
    server1->node_id(),
    server2->node_id(),
    session_key);

  server1->add_session(server2->address(), session);

  auto reverce_session = std::make_shared<vds::dht::network::dht_session>(
    server2->address(),
    server2->node_id(),
    server1->node_id(),
    session_key);

  server1->add_session(server1->address(), reverce_session);
}

void transport_hab::register_message(const vds::service_provider& sp, const vds::const_data_buffer& source_node_id,
  const vds::dht::network::imessage_map::message_info_t& message_info) {
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->message_log_.push_back(message_log_t(source_node_id, message_info));
}

void transport_hab::walk_messages(const std::function<message_log_action(const message_log_t&)>& callback) {
  auto p = this->message_log_.begin();
  while(this->message_log_.end() != p) {
    if(callback(*p) == message_log_action::remove) {
      p = this->message_log_.erase(p);
    }
    else {
      ++p;
    }
  }
}

test_server::test_server(const vds::network_address & address, const std::shared_ptr<transport_hab> & hab)
: logger_(
  test_config::instance().log_level(),
  test_config::instance().modules()),
  server_(address, hab),
  sp_(vds::service_provider::empty()){
}

void test_server::start(const std::shared_ptr<transport_hab> & hab, int index) {
  auto folder = vds::foldername(
    vds::foldername(vds::filename::current_process().contains_folder(), "servers"),
    std::to_string(index));
  folder.delete_folder(true);
  vds::foldername(folder, ".vds").create();


  registrator_.add(logger_);
  registrator_.add(mt_service_);
  registrator_.add(task_manager_);
  registrator_.add(server_);

  sp_ = registrator_.build("test_async_stream");

  auto root_folders = new vds::persistence_values();
  root_folders->current_user_ = folder;
  root_folders->local_machine_ = folder;
  sp_.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);

  registrator_.start(sp_);
  vds::mt_service::enable_async(sp_);
}

void test_server::stop() {
  registrator_.shutdown(sp_);
}

void test_server::add_sync_entry(const vds::const_data_buffer& object_id, uint32_t object_size) {
  this->server_.add_sync_entry(this->sp_, object_id, object_size);
}

vds::async_task<> test_server::process_datagram(const vds::service_provider& sp, const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id) {
  return this->server_.process_datagram(sp, datagram, source_node_id);
}

const vds::const_data_buffer& test_server::node_id() const {
  return this->server_.node_id();
}

const vds::network_address& test_server::address() const {
  return this->server_.address();
}

void test_server::add_session(const vds::network_address& address,
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  this->server_.add_session(this->sp_, address, session);
}

vds::async_task<> mock_server::process_datagram(
  const vds::service_provider& sp,
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id) {

  return this->sessions_[datagram.address()]->process_datagram(
    sp,
    this->transport_,
    vds::const_data_buffer(datagram.data(), datagram.data_size()));
}

void mock_server::add_session(
  const vds::service_provider& sp,
  const vds::network_address& address,
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  this->sessions_.emplace(address, session);
  this->client_->add_session(sp, session, 0);
}

const vds::const_data_buffer& mock_server::node_id() const {
  return this->client_.current_node_id();
}

const vds::network_address& mock_server::address() const {
  return this->address_;
}


#define route_client(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      return sp.get<vds::db_model>()->async_transaction(sp, [sp, message_info](vds::database_transaction & t) {\
        vds::binary_deserializer s(message_info.message_data());\
        vds::dht::messages::message_type message(s);\
        (*sp.get<vds::dht::network::client>())->apply_message(\
        sp.create_scope("messages::" #message_type),\
         t,\
         message,\
         message_info);\
        return true;\
      });\
      break;\
    }

vds::async_task<> mock_server::process_message(
  const vds::service_provider& sp,
  const message_info_t& message_info) {
  static_cast<mock_transport *>(this->transport_.get())->hab()->register_message(sp, this->node_id(), message_info);

  switch (message_info.message_type()) {
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

  default: {
      throw std::runtime_error("Invalid command");
    }
  }
  return vds::async_task<>::empty();

}

void mock_server::on_new_session(const vds::service_provider& sp, const vds::const_data_buffer& partner_id) {
  throw vds::vds_exceptions::invalid_operation();
}

void mock_server::add_sync_entry(
  const vds::service_provider& sp,
  const vds::const_data_buffer& object_id,
  uint32_t object_size) {
  sp.get<vds::db_model>()->async_transaction(sp, [this, sp, object_id, object_size](vds::database_transaction & t) {
    this->sync_process_.add_sync_entry(sp, t, object_id, object_size);
  }).wait();

}

mock_transport::mock_transport(mock_server * owner, const std::shared_ptr<transport_hab>& hab)
: owner_(owner), hab_(hab){
}

mock_server::mock_server(const vds::network_address & address, const std::shared_ptr<transport_hab>& hab)
: address_(address), transport_(new mock_transport(this, hab)) {
}

void mock_server::register_services(vds::service_registrator& registrator) {
  registrator.add_service<vds::db_model>(&this->db_model_);
  registrator.add_service<vds::dht::network::client>(&this->client_);
  registrator.add_service<vds::dht::network::imessage_map>(this);
}

void mock_server::start(const vds::service_provider& sp) {
  this->db_model_.start(sp);

  auto node_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  vds::asymmetric_public_key public_key(node_key);

  vds::certificate::create_options options;
  options.name = "Node Cert";
  options.country = "RU";
  options.organization = "IVySoft";
  auto node_cert = vds::certificate::create_new(public_key, node_key, options);

  this->client_.start(sp, node_cert, node_key, this->transport_);
}

void mock_server::stop(const vds::service_provider& sp) {
  this->db_model_.stop(sp);
}

vds::async_task<> mock_server::prepare_to_stop(const vds::service_provider& sp) {
  return vds::async_task<>::empty();
}

void mock_transport::start(
  const vds::service_provider& sp,
  const vds::certificate& node_cert,
  const vds::asymmetric_private_key& node_key,
  uint16_t port) {

  this->node_id_ = node_cert.fingerprint();

}

void mock_transport::stop(const vds::service_provider& sp) {
}

vds::async_task<> mock_transport::write_async(
  const vds::service_provider& sp,
  const vds::udp_datagram& datagram) {
  return this->hab_->write_async(sp, datagram, this->node_id_);
}

vds::async_task<> mock_transport::try_handshake(
  const vds::service_provider& sp,
  const std::string & address) {

  return vds::async_task<>::empty();
}

void mock_transport::add_session(
  const vds::service_provider& sp,
  const vds::network_address & address,
  const std::shared_ptr<vds::dht::network::dht_session> & session) {
  this->owner_->add_session(sp, address, session);
}
