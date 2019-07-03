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
    GTEST_FATAL_FAILURE_(std::system_error(error, std::system_category(), "Initiates Winsock").what());
  }
#endif

  std::vector<std::shared_ptr<test_server>> servers;

  auto hab = std::make_shared<transport_hab>();

  for(int i = 0; i < SERVER_COUNT; ++i) {
    GET_EXPECTED_GTEST(address, vds::network_address::udp_ip4("localhost", i));
    auto server = std::make_shared<test_server>(address, hab);
    CHECK_EXPECTED_GTEST(server->start(hab, i));
    servers.push_back(server);
    hab->add(server->address(), server.get());

    for(int j = 0; j < i; ++j) {
      CHECK_EXPECTED_GTEST(hab->attach(servers[j], server));
    }
  }

  vds::const_data_buffer object_data;
  object_data.resize(400000);
  for(size_t i = 0; i < object_data.size(); ++i) {
    object_data[i] = std::rand();
  }

  GET_EXPECTED_GTEST(object_id, vds::hash::signature(vds::hash::sha256(), object_data));
  CHECK_EXPECTED_GTEST(servers[4]->add_sync_entry(object_data));

  //All corresponding nodes have to approve data storage
  int stage = 0;
  size_t replica_count = 0;
  for(; ; ) {
    replica_count = 0;
    std::map<vds::const_data_buffer/*node*/, std::set<uint16_t /*replicas*/>> members;
    CHECK_EXPECTED_GTEST(hab->walk_messages([stage, &object_id, &members, &replica_count](const message_log_t & log_record)->vds::expected<message_log_action>{
      switch (log_record.message_info_.message_type()) {
      case vds::dht::network::message_type_t::sync_looking_storage_request: {
        vds::binary_deserializer s(log_record.message_info_.message_data());
        GET_EXPECTED(message, vds::message_deserialize<vds::dht::messages::sync_looking_storage_request>(s));
        if(message.object_id != object_id) {
          return vds::make_unexpected<std::runtime_error>("Invalid data");
        }
        members.emplace(log_record.target_node_id_, std::set<uint16_t>());
        break;
      }
      case vds::dht::network::message_type_t::sync_replica_operations_request: {
        vds::binary_deserializer s(log_record.message_info_.message_data());
        GET_EXPECTED(message, vds::message_deserialize<vds::dht::messages::sync_replica_operations_request>(s));
        if (message.object_id != object_id) {
          return vds::make_unexpected<std::runtime_error>("Invalid data");
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
        return vds::make_unexpected<std::runtime_error>("Invalid operation");
      }
      }

      return (stage == 0) ? message_log_action::skip : message_log_action::remove;
    }));

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
      CHECK_EXPECTED_GTEST(hab->walk_messages([stage, &object_id, &table, &server_index, &total_size](const message_log_t & log_record)->message_log_action {
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
      }));

      std::ofstream logfile("test.log", std::ofstream::out);
      print_table(logfile, table);
    }
    else {
      break;
    }

    bool is_ready_to_stop = true;
    for (const auto server : servers) {
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
    CHECK_EXPECTED_GTEST(server->stop());
  }

  GTEST_ASSERT_EQ(stage, 1);
}

vds::async_task<vds::expected<void>> transport_hab::write_async(
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer & source_node_id,
  const vds::network_address & source_address) {
  auto p = this->servers_.find(datagram.address());
  if(this->servers_.end() == p) {
    auto a = datagram.address().to_string();
    co_return vds::make_unexpected<std::runtime_error>("Invalid address " + a);
  }
  else {
    p->second->process_datagram(datagram, source_node_id, source_address);
  }
  co_return vds::expected<void>();
}

void transport_hab::add(const vds::network_address& address, test_server* server) {
  this->servers_[address] = server;
}

vds::expected<void> transport_hab::attach(const std::shared_ptr<test_server>& server1, const std::shared_ptr<test_server>& server2) {
  vds::const_data_buffer session_key;
  session_key.resize(32);
  vds::crypto_service::rand_bytes(session_key.data(), session_key.size());

  auto session = std::make_shared<vds::dht::network::dht_session>(
    server1->sp_,
    server2->address(),
    server1->node_id(),
    server2->node_id(),
    session_key);

  CHECK_EXPECTED(server1->add_session(session));

  auto reverce_session = std::make_shared<vds::dht::network::dht_session>(
    server2->sp_,
    server1->address(),
    server2->node_id(),
    server1->node_id(),
    session_key);

  CHECK_EXPECTED(server2->add_session(reverce_session));

  return vds::expected<void>();
}

void transport_hab::register_message(
  const vds::const_data_buffer& target_node_id,
  const vds::dht::network::imessage_map::message_info_t& message_info) {
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->message_log_.push_back(message_log_t(target_node_id, message_info));
}

vds::expected<void> transport_hab::walk_messages(const std::function<vds::expected<message_log_action>(const message_log_t&)>& callback) {
  auto p = this->message_log_.begin();
  while(this->message_log_.end() != p) {
    GET_EXPECTED(action, callback(*p));
    if(message_log_action::remove == action) {
      p = this->message_log_.erase(p);
    }
    else {
      ++p;
    }
  }
  return vds::expected<void>();
}

test_server::test_server(const vds::network_address & address, const std::shared_ptr<transport_hab> & hab)
: logger_(
  test_config::instance().log_level(),
  test_config::instance().modules()),
  server_(address, hab),
  sp_(nullptr){
}

vds::expected<void> test_server::start(const std::shared_ptr<transport_hab> & hab, int index) {
  GET_EXPECTED(current_process, vds::filename::current_process());
  auto folder = vds::foldername(
    vds::foldername(current_process.contains_folder(), "servers"),
    std::to_string(index));
  CHECK_EXPECTED(folder.delete_folder(true));
  CHECK_EXPECTED(folder.create());

  this->task_manager_.disable_timers();

  registrator_.add(logger_);
  registrator_.add(mt_service_);
  registrator_.add(task_manager_);
  registrator_.add(server_);

  registrator_.current_user(folder);
  registrator_.local_machine(folder);

  GET_EXPECTED_VALUE(this->sp_, registrator_.build());

  CHECK_EXPECTED(registrator_.start());
  
  this->process_thread_.reset(new vds::thread_apartment(this->sp_));
  return vds::expected<void>();
}

vds::expected<void> test_server::stop() {
  return registrator_.shutdown();
}

bool test_server::is_ready_to_stop() const {
  return this->process_thread_->is_ready_to_stop();
}

vds::expected<void> test_server::add_sync_entry(const vds::const_data_buffer& object_data) {
  return this->server_.add_sync_entry(object_data);
}

void test_server::process_datagram(
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id,
  const vds::network_address & source_address) {

  this->process_thread_->schedule([this, datagram, source_node_id, source_address]() -> vds::expected<void> {
    return this->server_.process_datagram(
        datagram,
        source_node_id,
        source_address).get();
  });
}

const vds::const_data_buffer& test_server::node_id() const {
  return (*this->sp_->get<vds::dht::network::client>())->current_node_id();
}

const vds::network_address& test_server::address() const {
  return this->server_.address();
}

vds::expected<void> test_server::add_session(
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  return this->server_.add_session(session).get();
}

vds::async_task<vds::expected<void>> mock_sync_server::process_datagram(
  
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id,
  const vds::network_address & source_address) {

  return this->sessions_[source_address]->process_datagram(
    this->transport_,
    vds::const_data_buffer(datagram.data(), datagram.data_size()));
}

vds::async_task<vds::expected<void>> mock_sync_server::add_session(
  const std::shared_ptr<vds::dht::network::dht_session>& session) {
  this->sessions_.emplace(session->address(), session);

  return (*this->sp_->get<vds::dht::network::client>())->add_session(session, 0);
}

const vds::network_address& mock_sync_server::address() const {
  return this->address_;
}


#define route_client(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      bool result;\
      CHECK_EXPECTED_ASYNC(co_await this->sp_->get<vds::db_model>()->async_transaction([sp = this->sp_, message_info, &final_tasks, &result](vds::database_transaction & t)->vds::expected<void> {\
        vds::binary_deserializer s(message_info.message_data()); \
        GET_EXPECTED(message, vds::message_deserialize<vds::dht::messages::message_type>(s)); \
        GET_EXPECTED_VALUE(result, (*sp->get<vds::dht::network::client>())->apply_message(\
          t, \
          final_tasks, \
          message, \
          message_info)); \
        return vds::expected<void>();\
      }));\
      co_return vds::expected<bool>(result);\
    }

#define route_client_wait(message_type)\
  case vds::dht::network::message_type_t::message_type: {\
      vds::binary_deserializer s(message_info.message_data());\
      GET_EXPECTED_ASYNC(message, vds::message_deserialize<vds::dht::messages::message_type>(s));\
      GET_EXPECTED_ASYNC(result, co_await (*this->sp_->get<vds::dht::network::client>())->apply_message(\
        message,\
        message_info));\
      co_return vds::expected<bool>(result);\
      break;\
    }

vds::async_task<vds::expected<bool>> mock_sync_server::process_message(
  message_info_t message_info) {

  static_cast<mock_transport *>(this->transport_.get())->hab()->register_message(
    this->sp_->get<vds::dht::network::client>()->current_node_id(), message_info);

  std::list<std::function<vds::async_task<vds::expected<void>>()>> final_tasks;

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

      route_client_wait(dht_find_node)
      route_client_wait(dht_find_node_response)
      route_client_wait(dht_ping)
      route_client_wait(dht_pong)

  default: {
      co_return vds::make_unexpected<std::runtime_error>("Invalid command");
    }
  }

  while (!final_tasks.empty()) {
    CHECK_EXPECTED_ASYNC(co_await final_tasks.front()());
    final_tasks.pop_front();
  }

  co_return vds::expected<bool>(false);

}

vds::async_task<vds::expected<void>> mock_sync_server::on_new_session(vds::const_data_buffer partner_id) {
  return vds::make_unexpected<vds::vds_exceptions::invalid_operation>();
}


vds::expected<void> mock_sync_server::add_sync_entry(
  const vds::const_data_buffer& object_data) {

  std::list<std::function<vds::async_task<vds::expected<void>>()>> final_tasks;

  CHECK_EXPECTED(this->sp_->get<vds::db_model>()->async_transaction([sp = this->sp_, object_data, &final_tasks](vds::database_transaction & t) -> vds::expected<void> {
    auto client = sp->get<vds::dht::network::client>();
    GET_EXPECTED(object_id, vds::hash::signature(vds::hash::sha256(), object_data));
    CHECK_EXPECTED((*client)->save_data(sp, t, object_id, object_data));
    vds::orm::chunk_dbo t1;
    CHECK_EXPECTED(t.execute(
      t1.insert(
        t1.object_id = object_id,
        t1.last_sync = std::chrono::system_clock::now() - std::chrono::hours(24)
      )));
    return (*client)->add_sync_entry(t, final_tasks, object_id, object_data.size());
  }).get());

  while (!final_tasks.empty()) {
    CHECK_EXPECTED(final_tasks.front()().get());
    final_tasks.pop_front();
  }

  return vds::expected<void>();
}

mock_transport::mock_transport(mock_sync_server * owner, const std::shared_ptr<transport_hab>& hab)
: owner_(owner), hab_(hab){
}

mock_sync_server::mock_sync_server(const vds::network_address & address, const std::shared_ptr<transport_hab>& hab)
: address_(address), transport_(new mock_transport(this, hab)) {
}

vds::expected<void> mock_sync_server::register_services(vds::service_registrator& registrator) {
  registrator.add_service<vds::db_model>(&this->db_model_);
  CHECK_EXPECTED(this->client_.register_services(registrator));
  registrator.add_service<vds::dht::network::imessage_map>(this);

  return vds::expected<void>();
}

vds::expected<void> mock_sync_server::start(const vds::service_provider * sp) {
  this->sp_ = sp;
  CHECK_EXPECTED(this->db_model_.start(sp));
  CHECK_EXPECTED(this->client_.start(sp, this->transport_, 0, true));

  return vds::expected<void>();
}

vds::expected<void> mock_sync_server::stop() {
  CHECK_EXPECTED(this->client_.stop());
  CHECK_EXPECTED(this->db_model_.stop());

  return vds::expected<void>();
}

vds::async_task<vds::expected<void>> mock_sync_server::prepare_to_stop() {
  co_return vds::expected<void>();
}

vds::async_task<vds::expected<void>> mock_transport::start(
  const vds::service_provider * /*sp*/,
  const std::shared_ptr<vds::asymmetric_public_key> & node_public_key,
  const std::shared_ptr<vds::asymmetric_private_key> & /*node_key*/,
  uint16_t /*port*/, bool dev_network) {
  GET_EXPECTED_VALUE_ASYNC(this->node_id_, node_public_key->hash(vds::hash::sha256()));
  co_return vds::expected<void>();
}

void mock_transport::stop() {
}

vds::async_task<vds::expected<void>> mock_transport::write_async(
  const vds::udp_datagram& datagram) {
  return this->hab_->write_async(datagram, this->node_id_, this->owner_->address());
}

vds::async_task<vds::expected<void>> mock_transport::try_handshake(
  const std::string & address) {
  co_return vds::expected<void>();
}

