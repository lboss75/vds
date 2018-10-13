#include "stdafx.h"
#include "test_sync_process.h"
#include "db_model.h"
#include "asymmetriccrypto.h"
#include "udp_socket.h"
#include "../../libs/vds_dht_network/private/dht_session.h"
#include "../../libs/vds_dht_network/private/dht_network_client_p.h"
#include "dht_network.h"
#include "messages/sync_messages.h"
#include "chunk_dbo.h"
#include "test_log.h"

#define SERVER_COUNT 100

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

  uint8_t object_id_data[32] = {
    0xC9, 0x9F, 0x11, 0x85,
    0xFE, 0x2E, 0x42, 0x7F,
    0xA7, 0x08, 0x11, 0xD2,
    0xDF, 0xC8, 0x91, 0xF6
  };
  vds::const_data_buffer object_id(object_id_data, sizeof(object_id_data));
  vds::const_data_buffer object_data;
  object_data.resize(400000);
  for(size_t i = 0; i < object_data.size(); ++i) {
    object_data[i] = std::rand();
  }
  servers[4]->add_sync_entry(object_id, object_data);

  //All corresponding nodes have to approove data storage
  int stage = 0;
  size_t replica_count = 0;
  for(; ; ) {
    replica_count = 0;
    std::map<vds::const_data_buffer/*node*/, std::set<uint16_t /*replicas*/>> members;
    hab->walk_messages([stage, &object_id, &members, &replica_count](const message_log_t & log_record)->message_log_action{
      switch (log_record.message_info_.message_type()) {
      case vds::dht::network::message_type_t::sync_looking_storage_request: {
        vds::binary_deserializer s(log_record.message_info_.message_data());
        auto message = vds::message_deserialize<vds::dht::messages::sync_looking_storage_request>(s);
        if(message.object_id != object_id) {
          throw std::runtime_error("Invalid data");
        }
        members.emplace(log_record.target_node_id_, std::set<uint16_t>());
        break;
      }
      case vds::dht::network::message_type_t::sync_replica_operations_request: {
        vds::binary_deserializer s(log_record.message_info_.message_data());
        auto message = vds::message_deserialize<vds::dht::messages::sync_replica_operations_request>(s);
        if (message.object_id != object_id) {
          throw std::runtime_error("Invalid data");
        }
        switch (message.message_type) {
        case vds::orm::sync_message_dbo::message_type_t::add_replica: {
          if (members.at(message.member_node).end() == members.at(message.member_node).find(message.replica)) {
            members.at(message.member_node).emplace(message.replica);
            ++replica_count;
          }
          break;
        }
        }
        break;
      }
      case vds::dht::network::message_type_t::dht_ping:
      case vds::dht::network::message_type_t::dht_pong:
      case vds::dht::network::message_type_t::dht_find_node:
      case vds::dht::network::message_type_t::dht_find_node_response:
      case vds::dht::network::message_type_t::sync_looking_storage_response:
      case vds::dht::network::message_type_t::sync_snapshot_response:
      case vds::dht::network::message_type_t::sync_leader_broadcast_response:
      case vds::dht::network::message_type_t::sync_replica_query_operations_request:
      case vds::dht::network::message_type_t::sync_replica_operations_response:
      case vds::dht::network::message_type_t::sync_replica_data:
      case vds::dht::network::message_type_t::sync_add_message_request:
      {
        break;
      }
      default: {
        throw std::runtime_error("Invalid operation");
      }
      }

      return (stage == 0) ? message_log_action::skip : message_log_action::remove;
    });

    if(stage == 0) {
      //valudate member count
      if(replica_count >= vds::dht::network::service::GENERATE_DISTRIBUTED_PIECES) {
        stage = 1;
      }

      //Dump
      std::map<std::string, std::string> server_index;
      for (int i = 0; i < SERVER_COUNT; ++i) {
        server_index[vds::base64::from_bytes(servers[i]->node_id())] = std::to_string(i);
      }
      std::list<std::tuple<std::string, std::map<std::string, std::string>>> table;
      size_t total_size = 0;
      hab->walk_messages([stage, &object_id, &table, &server_index, &total_size](const message_log_t & log_record)->message_log_action {
        std::map<std::string, std::string> columns;
        columns[server_index[vds::base64::from_bytes(log_record.target_node_id_)]] = "O";
        columns[server_index[vds::base64::from_bytes(log_record.message_info_.source_node())]] = "*";
        columns["Diff"] = std::to_string(log_record.message_info_.message_data().size());
        columns["Total"] = std::to_string(total_size);
        total_size += log_record.message_info_.message_data().size();
        table.push_back(std::make_tuple(
          std::to_string(table.size() + 1)
          + ". "
          + std::to_string(log_record.message_info_.message_type()),
          columns));

        return message_log_action::skip;
      });

      std::ofstream logfile("test.log", std::ofstream::out);
      print_table(logfile, table);
    }
    else {
      break;
    }

    bool is_ready_to_stop = true;
    for (auto server : servers) {
      if(!server->is_ready_to_stop()) {
        is_ready_to_stop = false;
        break;
      }
    }

    if(is_ready_to_stop) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  for(auto server : servers){
    server->stop();
  }

  GTEST_ASSERT_EQ(stage, 1);
}

std::future<void> transport_hab::write_async(
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer & source_node_id,
  const vds::network_address & source_address) {
  auto p = this->servers_.find(datagram.address());
  if(this->servers_.end() == p) {
    auto a = datagram.address().to_string();
    throw std::runtime_error("Invalid address " + a);
  }
  else {
    co_return co_await p->second->process_datagram(datagram, source_node_id, source_address);
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
    server1->sp_,
    server2->address(),
    server1->node_id(),
    server2->node_id(),
    session_key);

  server1->add_session(session);

  auto reverce_session = std::make_shared<vds::dht::network::dht_session>(
    server2->sp_,
    server1->address(),
    server2->node_id(),
    server1->node_id(),
    session_key);

  server2->add_session(reverce_session);
}

void transport_hab::register_message(
  const vds::const_data_buffer& target_node_id,
  const vds::dht::network::imessage_map::message_info_t& message_info) {
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->message_log_.push_back(message_log_t(target_node_id, message_info));
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
  sp_(nullptr){
}

void test_server::start(const std::shared_ptr<transport_hab> & hab, int index) {
  auto folder = vds::foldername(
    vds::foldername(vds::filename::current_process().contains_folder(), "servers"),
    std::to_string(index));
  folder.delete_folder(true);
  vds::foldername(folder, ".vds").create();

  this->task_manager_.disable_timers();

  registrator_.add(logger_);
  registrator_.add(mt_service_);
  registrator_.add(task_manager_);
  registrator_.add(server_);

  registrator_.current_user(folder);
  registrator_.local_machine(folder);

  this->sp_ = registrator_.build();

  registrator_.start();
  
  this->process_thread_.reset(new vds::thread_apartment(this->sp_));
}

void test_server::stop() {
  registrator_.shutdown();
}

bool test_server::is_ready_to_stop() const {
  return this->process_thread_->is_ready_to_stop();
}

void test_server::add_sync_entry(const vds::const_data_buffer& object_id, const vds::const_data_buffer& object_data) {
  this->server_.add_sync_entry(object_id, object_data);
}

std::future<void> test_server::process_datagram(
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id,
  const vds::network_address & source_address) {

  auto r = std::make_shared<std::promise<void>>();
  this->process_thread_->schedule([r, this, datagram, source_node_id, source_address]() {
    try {
      this->server_.process_datagram(
        datagram,
        source_node_id,
        source_address).get();
    }
    catch (...) {
      r->set_exception(std::current_exception());
      return;
    }

    r->set_value();
  });

  return r->get_future();
}

const vds::const_data_buffer& test_server::node_id() const {
  return (*this->sp_->get<vds::dht::network::client>())->current_node_id();
}

const vds::network_address& test_server::address() const {
  return this->server_.address();
}

void test_server::add_session(
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  this->server_.add_session(session);
}

std::future<void> mock_server::process_datagram(
  
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id,
  const vds::network_address & source_address) {

  return this->sessions_[source_address]->process_datagram(
    this->transport_,
    vds::const_data_buffer(datagram.data(), datagram.data_size()));
}

void mock_server::add_session(
  
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  this->sessions_.emplace(session->address(), session);

  (*this->sp_->get<vds::dht::network::client>())->add_session(session, 0);
}

const vds::network_address& mock_server::address() const {
  return this->address_;
}


#define route_client(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      co_return co_await this->sp_->get<vds::db_model>()->async_transaction([sp = this->sp_, message_info](vds::database_transaction & t) {\
        vds::binary_deserializer s(message_info.message_data());\
        auto message = vds::message_deserialize<vds::dht::messages::message_type>(s);\
        (*sp->get<vds::dht::network::client>())->apply_message(\
         t,\
         message,\
         message_info);\
        return true;\
      });\
      break;\
    }

#define route_client_wait(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      vds::binary_deserializer s(message_info.message_data());\
      auto message = vds::message_deserialize<vds::dht::messages::message_type>(s);\
      co_return co_await (*this->sp_->get<vds::dht::network::client>())->apply_message(\
        message,\
        message_info);\
      break;\
    }

#define route_client_nowait(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      vds::binary_deserializer s(message_info.message_data());\
      auto message = vds::message_deserialize<vds::dht::messages::message_type>(s);\
      (*this->sp_->get<vds::dht::network::client>())->apply_message(\
        message,\
        message_info);\
      break;\
    }

std::future<void> mock_server::process_message(
  
  const message_info_t& message_info) {

  static_cast<mock_transport *>(this->transport_.get())->hab()->register_message(
    this->sp_->get<vds::dht::network::client>()->current_node_id(), message_info);

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

      route_client_nowait(dht_find_node)
      route_client_wait(dht_find_node_response)
      route_client_nowait(dht_ping)
      route_client_nowait(dht_pong)

  default: {
      throw std::runtime_error("Invalid command");
    }
  }
  co_return;

}

std::future<void> mock_server::on_new_session( const vds::const_data_buffer& partner_id) {
  throw vds::vds_exceptions::invalid_operation();
}


void mock_server::add_sync_entry(
  
  const vds::const_data_buffer& object_id,
  const vds::const_data_buffer& object_data) {
  this->sp_->get<vds::db_model>()->async_transaction([sp = this->sp_, object_id, object_data](vds::database_transaction & t) {
    auto client = sp->get<vds::dht::network::client>();
    const auto replica_hash = vds::hash::signature(vds::hash::sha256(), object_data);
    auto fn = (*client)->save_data(sp, t, replica_hash, object_data);
    vds::orm::chunk_dbo t1;
    t.execute(
      t1.insert(
        t1.object_id = object_id,
        t1.replica_hash = replica_hash,
        t1.last_sync = std::chrono::system_clock::now() - std::chrono::hours(24)
      ));
    (*client)->add_sync_entry(t, object_id, object_data.size());
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
  this->client_.register_services(registrator);
  registrator.add_service<vds::dht::network::imessage_map>(this);
}

void mock_server::start(const vds::service_provider * sp) {
  this->sp_ = sp;
  this->db_model_.start(sp);
  this->client_.start(sp, this->transport_, 0);
}

void mock_server::stop() {
  this->client_.stop();
  this->db_model_.stop();
}

std::future<void> mock_server::prepare_to_stop() {
  co_return;
}

void mock_transport::start(
  const vds::service_provider * /*sp*/,
  const std::shared_ptr<vds::certificate> & node_cert,
  const std::shared_ptr<vds::asymmetric_private_key> & /*node_key*/,
  uint16_t /*port*/) {
  this->node_id_ = node_cert->fingerprint();
}

void mock_transport::stop() {
}

std::future<void> mock_transport::write_async(
  const vds::udp_datagram& datagram) {
  return this->hab_->write_async(datagram, this->node_id_, this->owner_->address());
}

std::future<void> mock_transport::try_handshake(
  const std::string & address) {
  co_return;
}

