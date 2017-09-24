/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_mock.h"
#include "test_config.h"

vds_mock::vds_mock()
{
}

vds_mock::~vds_mock()
{
  this->stop();
}

void vds_mock::start(size_t server_count)
{
  auto servers_folder = vds::foldername(vds::filename::current_process().contains_folder(), "servers");
  servers_folder.delete_folder(true);
  
  auto clients_folder = vds::foldername(vds::filename::current_process().contains_folder(), "clients");
  clients_folder.delete_folder(true);
            
  this->root_password_ = generate_password();
  int first_port = 8050;


  for (size_t i = 0; i < server_count; ++i) {
    if(0 < i){
      std::cout << "Initing server " << i << "\n";
      mock_client client(i);
      client.init_server(this->root_password_, "127.0.0.1", first_port + 1, first_port + 1);
    }
    
    std::unique_ptr<mock_server> server(new mock_server(i, first_port + 1, first_port + 1));
    try {
      if (0 == i) {
        std::cout << "Initing root\n";
        server->init_root(this->root_password_, first_port, first_port);
      }
      std::cout << "Starring server " << i << "\n";
      server->start();
    }
    catch (...) {
      std::cout << "Error...\n";
      try {
        server->stop();
      }
      catch (...) {
      }

      throw;
    }

    this->servers_.push_back(std::move(server));
  }
}

void vds_mock::stop()
{
  for(auto & p : this->servers_) {
    p->stop();
  }
}

void vds_mock::sync_wait()
{
  for(int i = 0; i < 1000; ++i){
    std::cout << "Waiting to synchronize...\n";
    
    bool is_good = true;
    vds::guid last_log_record;
    int index = 0;
    for(auto & p : this->servers_) {
      std::cout << "State[" << index ++ << "]: " << p->last_log_record().str() << "\n";
      if(!last_log_record){
        last_log_record = p->last_log_record();
      }
      else if(last_log_record != p->last_log_record()){
        is_good = false;
      }
    }
    
    if(is_good){
      return;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  
  std::cout << "Synchronize error\n";
  
  int index = 0;
  for(auto & p : this->servers_) {
    std::cout << "State[" << index ++ << "]: " << p->last_log_record().str() << "\n";
  }
  
  throw std::runtime_error("Synchronize error");
}

void vds_mock::upload_file(size_t client_index, const std::string & name, const void * data, size_t data_size)
{
  mock_client client(client_index);
  client.upload_file("root", this->root_password_, name, data, data_size);
}

vds::const_data_buffer vds_mock::download_data(size_t client_index, const std::string & name)
{
  mock_client client(client_index);
  return client.download_data("root", this->root_password_, name);
}

std::string vds_mock::generate_password(size_t min_len, size_t max_len)
{
  size_t password_len = 0;
  while (password_len < min_len || password_len > max_len) {
    password_len = ((size_t)std::rand() % (max_len + 1));
  }

  std::string result;
  while (result.length() < password_len) {
    result += '0' + ((size_t)std::rand() % ('z' - '0'));
  }

  return result;
}

mock_client::mock_client(int index)
  : index_(index)
{
}

void mock_client::init_server(
  const std::string& root_password,
  const std::string& address,
  int tcp_port,
  int udp_port)
{
  vds::service_registrator registrator;

  vds::file_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::mt_service mt_service;
  vds::crypto_service crypto_service;
  vds::task_manager task_manager;
  vds::network_service network_service;
  vds::client client("https://127.0.0.1:8050");

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "clients"), std::to_string(this->index_));
  folder.delete_folder(true);
  folder.create();

  registrator.add(logger);
  registrator.add(mt_service);
  registrator.add(crypto_service);
  registrator.add(task_manager);
  registrator.add(network_service);
  registrator.add(client);

  vds::barrier b;
  std::shared_ptr<std::exception> error;

  auto sp = registrator.build(("mock client on port " + std::to_string(tcp_port)).c_str());
  sp.set_property<vds::unhandled_exception_handler>(
    vds::service_provider::property_scope::any_scope,
    new vds::unhandled_exception_handler(
      [&b, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    error = ex;
    b.set();
  }));

  auto root_folders = new vds::persistence_values();
  root_folders->current_user_ = folder;
  root_folders->local_machine_ = folder;
  sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  
  try {
    registrator.start(sp);
    
    sp.get<vds::iclient>()->init_server(sp, "root", root_password)
      .wait(
        [&b, this](
          const vds::service_provider & sp,
          const vds::certificate & server_certificate,
          const vds::asymmetric_private_key & private_key) {
            auto root_folder = vds::foldername(vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "servers"), std::to_string(this->index_)), ".vds");
            root_folder.create();
            server_certificate.save(vds::filename(root_folder, "server.crt"));
            private_key.save(vds::filename(root_folder, "server.pkey"));
          b.set();
        },
          [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
            b.set();
        }, sp);
    b.wait();
  }
  catch (...) {
    try { registrator.shutdown(sp); }
    catch (...) {}

    throw;
  }

  registrator.shutdown(sp);

  if (error) {
    throw *error;
  }
}

void mock_client::upload_file(
  const std::string & login,
  const std::string & password,
  const std::string & name,
  const void * data,
  size_t data_size)
{
  this->start_vds(true, [login, password, name, data, data_size](const vds::service_provider&sp) {

    vds::foldername tmp_folder(vds::persistence::current_user(sp), "tmp");
    tmp_folder.create();
    vds::filename tmp_file(tmp_folder, "source");

    vds::dataflow(
      vds::dataflow_arguments<uint8_t>((const uint8_t *)data, data_size),
      vds::file_write(tmp_file, vds::file::file_mode::create_new)
    )
    (
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { sp.unhandled_exception(ex); },
      sp
      );

    vds::barrier b;
    sp.get<vds::iclient>()->upload_file(sp, login, password, name, tmp_file)
      .wait(
        [&b](const vds::service_provider&sp, const std::string& /*version_id*/) {
          b.set(); 
        },
        [&b](const vds::service_provider&sp, const std::shared_ptr<std::exception> & ex) {
          b.set();
          sp.unhandled_exception(ex);
        },
        sp);

    b.wait();
  }, false);
}

vds::const_data_buffer mock_client::download_data(const std::string & login, const std::string & password, const std::string & name)
{
  std::shared_ptr<std::exception> error;
  std::vector<uint8_t> result;
  this->start_vds(true, [&result, &error, login, password, name](const vds::service_provider&sp) {
    vds::barrier b;
    
    vds::foldername tmp_folder(vds::persistence::current_user(sp), "tmp");
    tmp_folder.create();
    vds::filename tmp_file(tmp_folder, "target");

    for(int try_count = 0; ; ++try_count){
      sp.get<vds::iclient>()->download_data(sp, login, password, name, tmp_file)
      .wait(
        [&b](const vds::service_provider & sp, const vds::guid & version_id){
          b.set();
        },
        [&b, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          error = ex;
          b.set();
        },
        sp);

      b.wait();
      b.reset();
      if (error) {
        std::cout << "[" << try_count << "]:" << error->what() << "\n";
        if(try_count < 10){
          std::this_thread::sleep_for(std::chrono::seconds(5));
          error.reset();
          continue;
        }
        return;
      }
      else {
        break;
      }
    }

    vds::dataflow(
      vds::file_read(tmp_file),
      vds::collect_data(result)
    )
    (
      [](const vds::service_provider & sp) {
      },
      [&error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        error = ex; 
      },
      sp);
  }, false);

  if (error) {
    throw *error;
  }

  return result;
}


void mock_client::start_vds(bool full_client, const std::function<void(const vds::service_provider&sp)> & handler, bool clear_folder)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;
  vds::client client("https://127.0.0.1:" + std::to_string(8050 + this->index_));
  vds::task_manager task_manager;

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "clients"), std::to_string(this->index_));
  if (clear_folder) {
    folder.delete_folder(true);
  }
  folder.create();

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(task_manager);
  registrator.add(network_service);
  registrator.add(crypto_service);
  
  if(full_client){
    registrator.add(client);
  }

  std::shared_ptr<std::exception> error;
  auto sp = registrator.build("mock client");
  sp.set_property<vds::unhandled_exception_handler>(
    vds::service_provider::property_scope::any_scope,
    new vds::unhandled_exception_handler(
      [&error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        error = ex;
  }));

  auto root_folders = new vds::persistence_values();
  root_folders->current_user_ = folder;
  root_folders->local_machine_ = folder;
  sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  registrator.start(sp);

  try {
    handler(sp);
  }
  catch (...) {
    try { registrator.shutdown(sp); }
    catch (...) {}

    throw;
  }

  registrator.shutdown(sp);

  if (error) {
    throw *error;
  }
}

mock_server::mock_server(int index, int tcp_port, int udp_port)
  : index_(index),
  tcp_port_(tcp_port),
  udp_port_(udp_port),
  sp_(vds::service_provider::empty()),
  logger_(
    test_config::instance().log_level(),
    test_config::instance().modules())
{
}

void mock_server::init_root(const std::string & root_password, int tcp_port, int udp_port)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;
  vds::client client("https://127.0.0.1:" + std::to_string(tcp_port));
  vds::task_manager task_manager;
  vds::server server;

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "servers"), std::to_string(0));
  folder.delete_folder(true);
  vds::foldername(folder, ".vds").create();

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(task_manager);
  registrator.add(crypto_service);
  registrator.add(network_service);
  registrator.add(server);

  std::shared_ptr<std::exception> error;

  auto sp = registrator.build("mock server::init_root");
  sp.set_property<vds::unhandled_exception_handler>(
    vds::service_provider::property_scope::any_scope,
    new vds::unhandled_exception_handler(
      [&error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        error = ex;
      }));
  try {
    auto root_folders = new vds::persistence_values();
    root_folders->current_user_ = folder;
    root_folders->local_machine_ = folder;
    sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);

    vds::asymmetric_private_key private_key(vds::asymmetric_crypto::rsa4096());
    private_key.generate();

    auto root_id = vds::guid::new_guid();
    vds::certificate root_certificate = vds::_certificate_authority::create_root_user(root_id, private_key);

    vds::asymmetric_private_key server_private_key(vds::asymmetric_crypto::rsa4096());
    server_private_key.generate();

    vds::guid current_server_id = vds::guid::new_guid();
    vds::certificate server_certificate = vds::certificate_authority::create_server(
      current_server_id,
      root_certificate,
      private_key,
      server_private_key);
    
    server_certificate.save(vds::filename(vds::foldername(folder, ".vds"), "server.crt"));
    server_private_key.save(vds::filename(vds::foldername(folder, ".vds"), "server.pkey"));
    
    server.set_port(tcp_port);

    registrator.start(sp);

    sp.get<vds::istorage_log>()->reset(
      sp,
      root_id,
      root_certificate,
      private_key,
      root_password,
      "https://127.0.0.1:" + std::to_string(tcp_port));
  }
  catch (...) {
    try { registrator.shutdown(sp); }
    catch (...) {}

    throw;
  }

  registrator.shutdown(sp);

  if (error) {
    throw *error;
  }
}


void mock_server::start()
{
  auto folder = vds::foldername(
    vds::foldername(vds::filename::current_process().contains_folder(), "servers"),
    std::to_string(this->index_));
  folder.create();
  
 
  this->registrator_.add(this->mt_service_);
  this->registrator_.add(this->logger_);
  this->registrator_.add(this->task_manager_);
  this->registrator_.add(this->network_service_);
  this->registrator_.add(this->crypto_service_);
  this->registrator_.add(this->server_);

  this->connection_manager_.set_addresses("udp://127.0.0.1:" + std::to_string(8050 + this->index_));
  this->registrator_.add(this->connection_manager_);
  this->registrator_.add(this->server_log_sync_);

  this->server_.set_port(8050 + this->index_);

  this->sp_ = this->registrator_.build(("mock server[" + std::to_string(this->index_) + "]").c_str());
  auto root_folders = new vds::persistence_values();
  root_folders->current_user_ = folder;
  root_folders->local_machine_ = folder;
  this->sp_.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  this->registrator_.start(this->sp_);
}

void mock_server::stop()
{
  this->registrator_.shutdown(this->sp_);
}

vds::guid mock_server::last_log_record() const
{
  return this->sp_.get<vds::istorage_log>()->get_last_applied_record(this->sp_);
}
