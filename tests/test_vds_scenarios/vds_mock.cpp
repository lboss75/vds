/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_mock.h"

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
      mock_client client(i);
      client.init_server(this->root_password_, "127.0.0.1", first_port + 1);
    }
    
    std::unique_ptr<mock_server> server(new mock_server(i, first_port + 1));
    try {
      if (0 == i) {
        server->init_root(this->root_password_, first_port);
      }
      server->start();
    }
    catch (...) {
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
  int port)
{
  this->start_vds(true, [root_password, address, port, this](const vds::service_provider&sp) {
    vds::barrier b;
    sp
      .get<vds::iclient>()
      .init_server("root", root_password)
      .wait(
        [&b, this](
          const vds::certificate & server_certificate,
          const vds::asymmetric_private_key & private_key) {
            auto root_folder = vds::foldername(vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "servers"), std::to_string(this->index_)), ".vds");
            root_folder.create();
            server_certificate.save(vds::filename(root_folder, "server.crt"));
            private_key.save(vds::filename(root_folder, "server.pkey"));
          b.set();
        },
          [&b](std::exception_ptr ex) {
        FAIL() << vds::exception_what(ex);
        b.set();
      });
    b.wait();
  },
  true);
}

void mock_client::upload_file(const std::string & login, const std::string & password, const std::string & name, const void * data, size_t data_size)
{
  this->start_vds(true, [login, password, name, data, data_size](const vds::service_provider&sp) {
    vds::barrier b;
    sp
      .get<vds::iclient>()
      .upload_file(login, password, name, data, data_size)
      .wait(
        [&b](const std::string& /*version_id*/) {
          b.set(); 
        },
        [&b](std::exception_ptr ex) {
          b.set();
          FAIL() << vds::exception_what(ex);
      });

    b.wait();
  }, false);
}

vds::const_data_buffer mock_client::download_data(const std::string & login, const std::string & password, const std::string & name)
{
  vds::const_data_buffer result;
  this->start_vds(true, [&result, login, password, name](const vds::service_provider&sp) {
    vds::barrier b;
    
    sp.get<vds::iclient>().download_data(login, password, name)
    .wait(
      [&result, &b](vds::const_data_buffer && data){ result = std::move(data); b.set();},
      [&result, &b](std::exception_ptr ex) {
        b.set();
        FAIL() << vds::exception_what(ex);
      }
    );
    
    b.wait();
  }, false);
  return result;
}


void mock_client::start_vds(bool full_client, const std::function<void(const vds::service_provider&sp)> & handler, bool clear_folder)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(vds::ll_trace);
  vds::crypto_service crypto_service;
  vds::client client;
  vds::task_manager task_manager;

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "clients"), std::to_string(this->index_));
  if (clear_folder) {
    folder.delete_folder(true);
  }
  folder.create();
  registrator.set_root_folders(folder, folder);

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(network_service);
  registrator.add(crypto_service);
  registrator.add(task_manager);
  
  if(full_client){
    registrator.add(client);
  }

  auto sp = registrator.build();
  try {
    handler(sp);
  }
  catch (...) {
    try { registrator.shutdown(); }
    catch (...) {}

    throw;
  }

  registrator.shutdown();
}

mock_server::mock_server(int index, int port)
  : index_(index),
  port_(port),
  logger_(vds::ll_trace)
{
}

void mock_server::init_root(const std::string & root_password, int port)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(vds::ll_trace);
  vds::crypto_service crypto_service;
  vds::client client;
  vds::task_manager task_manager;
  vds::server server(true);

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "servers"), std::to_string(0));
  folder.delete_folder(true);
  vds::foldername(folder, ".vds").create();
  registrator.set_root_folders(folder, folder);

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(network_service);
  registrator.add(crypto_service);
  registrator.add(task_manager);
  registrator.add(server);

  try {
    auto sp = registrator.build();
    
    vds::asymmetric_private_key private_key(vds::asymmetric_crypto::rsa4096());
    private_key.generate();

    vds::certificate root_certificate = vds::_certificate_authority::create_root_user(private_key);

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
    
    server.start(sp);
    
    sp.get<vds::istorage_log>().reset(
      root_certificate,
      private_key,
      root_password,
      "https://127.0.0.1:" + std::to_string(port));
  }
  catch (...) {
    try { registrator.shutdown(); }
    catch (...) {}

    throw;
  }

  registrator.shutdown();
}


void mock_server::start()
{
  auto folder = vds::foldername(
    vds::foldername(vds::filename::current_process().contains_folder(), "servers"),
    std::to_string(this->index_));
  folder.create();
  
  this->registrator_.set_root_folders(folder, folder);
  
  this->registrator_.add(this->mt_service_);
  this->registrator_.add(this->logger_);
  this->registrator_.add(this->network_service_);
  this->registrator_.add(this->task_manager_);
  this->registrator_.add(this->crypto_service_);
  this->registrator_.add(this->server_);
  this->registrator_.add(this->connection_manager_);
  this->registrator_.add(this->server_log_sync_);

  this->server_.set_port(8050 + this->index_);

  auto sp = this->registrator_.build();

  this->connection_manager_.start_servers("udp://127.0.0.1:" + std::to_string(8050 + this->index_));
}

void mock_server::stop()
{
  this->registrator_.shutdown();
}
