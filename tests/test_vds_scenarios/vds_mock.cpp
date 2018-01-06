/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_mock.h"
#include "test_config.h"
#include "storage_log.h"
#include "mt_service.h"
#include "leak_detect.h"
#include "private/server_p.h"
#include "file_operations.h"
#include "db_model.h"
#include "member_user.h"
#include "user_dbo.h"

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

  vds::leak_detect_resolver resolver;
  for (size_t i = 0; i < server_count; ++i) {
    if (0 == i) {
      std::cout << "Initing root\n";
      mock_server::init_root(i, first_port + i, this->root_password_);
    } else {
      std::cout << "Initing server " << i << "\n";
      mock_server::init(i, first_port + i, "root", this->root_password_);
    }

    std::unique_ptr<mock_server> server(new mock_server(i, first_port + i));
    try {
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

    resolver.add(&(*server).leak_detect_);
    this->servers_.push_back(std::move(server));
  }
  std::cout << "Leaks:" << resolver.resolve();
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

void vds_mock::upload_file(size_t client_index, const vds::guid &channel_id, const std::string &name,
                           const std::string &mimetype, const vds::filename &file_path) {
  vds::barrier b;
  std::shared_ptr<std::exception> error;

  this->servers_[client_index]->get<vds::file_manager::file_operations>()->upload_file(
      this->servers_[client_index]->get_service_provider(),
      channel_id,
      name,
      mimetype,
      file_path)
      .execute([&b, &error](const std::shared_ptr<std::exception> & ex){
    if(ex){
      error = ex;
    }
    b.set();
  });

  b.wait();
}

vds::const_data_buffer vds_mock::download_data(size_t client_index, const std::string &name) {
  throw std::runtime_error("Not implemented");
}

vds::user_channel vds_mock::create_channel(int index, const std::string &name) {
  vds::user_channel result;
  vds::barrier b;
  std::shared_ptr<std::exception> error;
  this->servers_[index]->get<vds::db_model>()->async_transaction(
      this->servers_[index]->get_service_provider(),
      [this, index, name, &result](vds::database_transaction & t) -> bool{

    vds::user_dbo t1;

    auto st = t.get_reader(
        t1
            .select(t1.id, t1.private_key)
            .where(t1.login == "root"));
    if(!st.execute()){
      throw std::runtime_error("Unable to load user root");
    }
    auto root_private_key = vds::asymmetric_private_key::parse_der(
        t1.private_key.get(st),
        this->root_password_);

    auto user_mng = this->servers_[index]->get<vds::user_manager>();
    auto root_user = user_mng->by_login(t, "root");

    auto read_private_key = vds::asymmetric_private_key::generate(
        vds::asymmetric_crypto::rsa4096());
    auto write_private_key = vds::asymmetric_private_key::generate(
        vds::asymmetric_crypto::rsa4096());

    vds::transaction_block log;
    result = user_mng->create_channel(log, <#initializer#>, <#initializer#>, root_user, root_private_key, name,
                                      read_private_key, write_private_key);

    return true;
  }).execute([&b, &error](const std::shared_ptr<std::exception> & ex){
    if(ex){
      error = ex;
    }
    b.set();
  });

  b.wait();
  if(error){
    throw std::runtime_error(error->what());
  }

  return result;
}

mock_server::mock_server(int index, int udp_port)
  : index_(index),
  tcp_port_(udp_port),
  udp_port_(udp_port),
  sp_(vds::service_provider::empty()),
  logger_(
    test_config::instance().log_level(),
    test_config::instance().modules())
{
  this->leak_detect_.name_ = "mock_server";
  this->leak_detect_.dump_callback_ = [this](vds::leak_detect_collector * collector){
    collector->add(this->server_);
  };
}

void mock_server::init_root(int index, int udp_port, const std::string & root_password)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;
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
	
    registrator.start(sp);

	vds::imt_service::enable_async(sp);
	vds::barrier b;
    server
        .reset(sp, "root", root_password, "test" + std::to_string(udp_port), udp_port)
		.execute([&error, &b](const std::shared_ptr<std::exception> & ex) {
		if (ex) {
			error = ex;
		}
		b.set();
	});

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

  //this->connection_manager_.set_addresses("udp://127.0.0.1:" + std::to_string(8050 + this->index_));

  this->sp_ = this->registrator_.build(("mock server on " + std::to_string(this->udp_port_)).c_str());
  auto root_folders = new vds::persistence_values();
  root_folders->current_user_ = folder;
  root_folders->local_machine_ = folder;
  this->sp_.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  this->registrator_.start(this->sp_);

  std::shared_ptr<std::exception> error;
  vds::barrier b;
  this->server_.start_network(this->sp_).execute([&b, &error](const std::shared_ptr<std::exception> & ex){
    if(ex){
      error = ex;
    }
    b.set();
  });
  b.wait();
  if(error){
    throw std::runtime_error(error->what());
  }
}

void mock_server::stop()
{
  this->registrator_.shutdown(this->sp_);
}

vds::guid mock_server::last_log_record() const
{
  return this->sp_.get<vds::istorage_log>()->get_last_applied_record(this->sp_);
}

void mock_server::init(int index, int udp_port, const std::string &user_name, const std::string &user_password) {
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger logger(
      test_config::instance().log_level(),
      test_config::instance().modules());
  vds::crypto_service crypto_service;
  vds::task_manager task_manager;
  vds::server server;

  auto folder = vds::foldername(
      vds::foldername(vds::filename::current_process().contains_folder(), "servers"),
      std::to_string(index));
  folder.delete_folder(true);
  vds::foldername(folder, ".vds").create();

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(task_manager);
  registrator.add(crypto_service);
  registrator.add(network_service);
  registrator.add(server);

  std::shared_ptr<std::exception> error;

  auto sp = registrator.build("mock server::init");
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

    registrator.start(sp);

    vds::imt_service::enable_async(sp);
    vds::barrier b;
    server
        .init_server(sp, user_name, user_password, "test" + std::to_string(udp_port), udp_port)
        .execute([&error, &b](const std::shared_ptr<std::exception> & ex) {
          if (ex) {
            error = ex;
          }
          b.set();
        });

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
