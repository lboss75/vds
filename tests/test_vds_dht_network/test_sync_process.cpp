#include "stdafx.h"
#include "test_sync_process.h"
#include "db_model.h"
#include "asymmetriccrypto.h"
#include "udp_socket.h"
#include "../../libs/vds_dht_network/private/dht_session.h"

#define SERVER_COUNT 100

TEST(test_vds_dht_network, test_sync_process) {
  std::vector<std::shared_ptr<test_server>> servers;

  auto hab = std::make_shared<transport_hab>();

  for(int i = 0; i < SERVER_COUNT; ++i) {
    auto server = std::make_shared<test_server>(hab);
    server->start(hab, i);
    servers.push_back(server);
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
  return this->servers_[datagram.address()]->process_datagram(sp, datagram, source_node_id);
}

vds::async_task<> transport_hab::try_handshake(
  const vds::service_provider& sp,
  const std::string& address,
  const vds::const_data_buffer & source_node_id) {
  auto addr = vds::network_address::parse(address);
  auto p = this->servers_.find(addr);
  if(this->servers_.end() == p) {
    return vds::async_task<>(std::make_shared<std::runtime_error>("Invalid address"));
  }

  return this->servers_[addr]->try_handshake(sp, source_node_id);
}

void transport_hab::add(const vds::network_address& address, test_server* server) {
  this->servers_[address] = server;
}

test_server::test_server(const std::shared_ptr<transport_hab> & hab)
: logger_(
  test_config::instance().log_level(),
  test_config::instance().modules()),
  server_(std::make_shared<mock_transport>(hab)),
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
  sp_.get<vds::db_model>()->async_transaction(sp_, [this, object_id, object_size](vds::database_transaction & t) {
    this->sync_process_.add_sync_entry(sp_, t, object_id, object_size);
  }).wait();
}

vds::async_task<> test_server::mock_server::process_datagram(
  const vds::service_provider& sp,
  const vds::udp_datagram& datagram,
  const vds::const_data_buffer& source_node_id) {

  return this->sessions_[datagram.address()]->process_datagram(
    sp,
    this->transport_,
    vds::const_data_buffer(datagram.data(), datagram.data_size()));
}

vds::async_task<> test_server::mock_server::try_handshake(
  const vds::service_provider& sp,
  const vds::const_data_buffer& source_node_id) {

  vds::network_address address;
  vds::const_data_buffer this_node_id;
  vds::const_data_buffer partner_node_id;
  vds::const_data_buffer session_key;

  auto session = std::make_shared<vds::dht::network::dht_session>(
    address,
    this_node_id,
    source_node_id,
    session_key);
}

test_server::mock_transport::mock_transport(const std::shared_ptr<transport_hab>& hab)
: hab_(hab){
}

void test_server::mock_server::register_services(vds::service_registrator& registrator) {
  registrator.add_service<vds::db_model>(&this->db_model_);
  registrator.add_service<vds::dht::network::client>(&this->client_);
}

void test_server::mock_server::start(const vds::service_provider& sp) {
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

void test_server::mock_server::stop(const vds::service_provider& sp) {
  this->db_model_.stop(sp);
}

vds::async_task<> test_server::mock_server::prepare_to_stop(const vds::service_provider& sp) {
  return vds::async_task<>::empty();
}

void test_server::mock_transport::start(
  const vds::service_provider& sp,
  const vds::certificate& node_cert,
  const vds::asymmetric_private_key& node_key,
  uint16_t port) {

  this->node_id_ = node_cert.fingerprint();

}

void test_server::mock_transport::stop(const vds::service_provider& sp) {
}

vds::async_task<> test_server::mock_transport::write_async(
  const vds::service_provider& sp,
  const vds::udp_datagram& datagram) {
  return this->hab_->write_async(sp, datagram, this->node_id_);
}

vds::async_task<> test_server::mock_transport::try_handshake(
  const vds::service_provider& sp,
  const std::string & address) {
  return this->hab_->try_handshake(sp, address, this->node_id_);

}
