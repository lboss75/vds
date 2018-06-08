/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_mock.h"
#include "test_config.h"
#include "private/server_p.h"
#include "file_operations.h"
#include "db_model.h"
#include "user_manager.h"
#include "channel_add_reader_transaction.h"
#include "channel_add_writer_transaction.h"
#include "dht_object_id.h"
#include "member_user.h"

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

  this->root_login_ = "root";
  this->root_password_ = generate_password();
  const auto first_port = 8050;

  for (size_t i = 0; i < server_count; ++i) {
    if (0 == i) {
      std::cout << "Initing root\n";
	    mock_server::init_root(i, first_port + i, this->root_password_);
    }
    //else {
    //  std::cout << "Initing server " << i << "\n";
    //  mock_server::init(i, first_port + i, this->root_login_, this->root_password_);
    //}

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
  std::cout << "Waiting to synchronize...\n";
  for(int i = 0; i < 1000; ++i){

    std::vector<vds::server_statistic> statistics;
    for (auto & p : this->servers_) {

      vds::barrier b;
      std::shared_ptr<std::exception> error;

      p->get_statistic()
        .execute([&b, &error, &statistics](
          const std::shared_ptr<std::exception> & ex,
          const vds::server_statistic & statistic) {
        if (ex) {
          error = ex;
        }
        else {
          statistics.push_back(statistic);
        }

        b.set();
      });
      b.wait();

      if (error) {
        throw std::runtime_error(error->what());
      }
    }

    std::cout << "order|                      id                    |";

    std::map<int, std::list<vds::const_data_buffer>> order_no;
    std::map<vds::const_data_buffer, std::map<int, char>> states;
    for(std::size_t i = 0; i < statistics.size(); ++i) {
      for (auto p : statistics[i].sync_statistic_.unknown_) {
        states[p][i] = '?';
      }

      for(auto leaf : statistics[i].sync_statistic_.leafs_) {

        auto & p = order_no[leaf.order_no];
        if(p.end() == std::find(p.begin(), p.end(), leaf.id)) {
          p.push_back(leaf.id);
        }
        switch ((vds::orm::transaction_log_record_dbo::state_t)leaf.state) {
        case vds::orm::transaction_log_record_dbo::state_t::processed:
          states[leaf.id][i] = '+';
          break;
        case vds::orm::transaction_log_record_dbo::state_t::leaf:
          states[leaf.id][i] = 'V';
          break;
        case vds::orm::transaction_log_record_dbo::state_t::invalid:
          states[leaf.id][i] = 'X';
          break;
        }
      }
      std::cout << i << "|";
    }
    std::cout << "\n";

    for (auto p : order_no) {
      for (auto record : p.second) {
        std::cout << std::setw(5) << p.first << "|" << std::setw(16) << vds::base64::from_bytes(record) << "|";
        for (std::size_t i = 0; i < statistics.size(); ++i) {
          auto p2 = states.find(record);
          if(states.end() == p2) {
            std::cout << " |";            
          }
          else {
            const auto p1 = p2->second.find(i);
            if (p2->second.end() == p1) {
              std::cout << " |";
            }
            else {
              std::cout << p1->second << "|";
            }
          }
        }
      }
      std::cout << "\n";
    }
    
    bool is_good = true;
    vds::sync_statistic last_sync_statistic;
    for(auto & p : statistics) {

      if(!last_sync_statistic){
        last_sync_statistic = p.sync_statistic_;
      }
      else if(last_sync_statistic != p.sync_statistic_){
        is_good = false;
      }
    }
    
    if(is_good){
      return;
    }

    //////////////////////////////////////////////////////////////////////
    std::map<vds::const_data_buffer, std::set<std::size_t>> all_replicas;
    for (std::size_t i = 0; i < statistics.size(); ++i) {
      for (auto replica : statistics[i].sync_statistic_.replicas_) {
        all_replicas[replica].emplace(i);
      }
      std::cout << i << "|";
    }
    std::cout << "\n";

    std::map<std::string, int> replicas;
    for(auto & p : all_replicas) {
      std::string mask;
      for (std::size_t i = 0; i < statistics.size(); ++i) {
        if(p.second.end() == p.second.find(i)) {
          mask += " |";
        }
        else {
          mask += "+|";
        }
      }

      replicas[mask]++;
    }

    for(auto & r : replicas) {
      std::cout << r.first << r.second << "\n";
    }
    //////////////////////////////////////////////////////////////////////
    std::map<std::string, std::set<std::size_t>> all_replica_dist;
    for (std::size_t i = 0; i < statistics.size(); ++i) {
      for (auto replica : statistics[i].sync_statistic_.replica_distribution_) {
        all_replica_dist[replica].emplace(i);
      }
      std::cout << i << "|";
    }
    std::cout << "\n";

    std::map<std::string, int> replica_dist;
    for (auto & p : all_replica_dist) {
      std::string mask;
      for (std::size_t i = 0; i < statistics.size(); ++i) {
        if (p.second.end() == p.second.find(i)) {
          mask += " |";
        }
        else {
          mask += "+|";
        }
      }

      replica_dist[mask]++;
    }

    for (auto & r : replica_dist) {
      std::cout << r.first << r.second << "\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  
  std::cout << "Synchronize error\n";
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

void vds_mock::allow_write_channel(size_t client_index, const vds::const_data_buffer &channel_id){
//  vds::barrier b;
//  std::shared_ptr<std::exception> error;
//
//  auto sp = this->servers_[client_index]->get_service_provider().create_scope(__FUNCTION__);
//  vds::mt_service::enable_async(sp);
//  this->servers_[client_index]
//      ->get<vds::db_model>()
//      ->async_transaction(
//          sp,
//          [this, client_index, channel_id, sp](vds::database_transaction & t)->bool {
//
//            auto user_mng = std::make_shared<vds::user_manager>();
//            user_mng->load(
//                sp,
//                t,
//                vds::dht::dht_object_id::from_user_email(this->root_login_),
//                vds::symmetric_key::from_password(this->root_password_),
//                vds::hash::signature(vds::hash::sha256(), this->root_password_.c_str(), this->root_password_.length())
//
//            std::string channel_name;
//            vds::certificate channel_read_cert;
//            vds::asymmetric_private_key channel_read_private_key;
//            vds::certificate channel_write_cert;
//            vds::asymmetric_private_key channel_write_private_key;
//            if(!user_mng->get_channel_write_certificate(
//                channel_id,
//                channel_name,
//                channel_read_cert,
//                channel_read_private_key,
//                channel_write_cert,
//                channel_write_private_key)){
//              throw std::runtime_error("Unable to get channel write certificate");
//            }
//			vds::user_channel channel(channel_id, channel_name, channel_read_cert, channel_write_cert);
//
//			sp.get<vds::logger>()->trace("MOCK", sp, "Allow write channel %s(%s). Cert %s",
//				channel_name.c_str(),
//				channel_id.str().c_str(),
//				vds::cert_control::get_id(channel_write_cert).str().c_str());
//
//            vds::asymmetric_private_key device_private_key;
//            auto device_user = user_mng->get_current_device(
//                sp,
//                device_private_key);
//
//            vds::transactions::transaction_block_builder log;
//
//			channel.add_reader(
//				log,
//				device_user,
//				root_user,
//				root_user_private_key,
//				channel_read_private_key);
//			channel.add_writer(
//					log,
//					device_user,
//					root_user,
//					root_user_private_key,
//					channel_write_private_key);
//
//            log.save(
//                sp,
//                t,
//                [sp, user_mng](const vds::guid & channel_id,
//                   vds::certificate & read_cert,
//                   vds::certificate & write_cert,
//                   vds::asymmetric_private_key & write_private_key){
//                  auto channel = user_mng->get_channel(sp, channel_id);
//                  if(!channel){
//                    throw std::runtime_error("Invalid channel");
//                  }
//                  read_cert = channel.read_cert();
//                  write_cert = channel.write_cert();
//                  write_private_key = user_mng->get_channel_write_key(sp, channel_id);
//                });
//            return true;
//          })
//      .execute([&b, &error](const std::shared_ptr<std::exception> & ex){
//        if(ex){
//          error = ex;
//        }
//        b.set();
//      });
//
//  b.wait();
//  if(error){
//    throw std::runtime_error(error->what());
//  }
}

void vds_mock::allow_read_channel(size_t client_index, const vds::const_data_buffer &channel_id){
//  vds::barrier b;
//  std::shared_ptr<std::exception> error;
//
//  auto sp = this->servers_[client_index]->get_service_provider().create_scope(__FUNCTION__);
//  vds::mt_service::enable_async(sp);
//  this->servers_[client_index]
//      ->get<vds::db_model>()
//      ->async_transaction(
//          sp,
//          [this, client_index, channel_id, sp](vds::database_transaction & t)->bool {
//
//            auto user_mng = this->servers_[client_index]->get<vds::user_manager>();
//            auto root_user = user_mng->import_user(*this->root_device_activation_.certificate_chain().rbegin());
//            vds::asymmetric_private_key root_user_private_key = this->root_device_activation_.private_key();
//
//            vds::security_walker walker(
//                root_user.id(),
//                root_user.user_certificate(),
//                root_user_private_key);
//            walker.load(sp, t);
//
//            std::string channel_name;
//            vds::certificate channel_read_cert;
//            vds::asymmetric_private_key channel_read_private_key;
//            vds::certificate channel_write_cert;
//            vds::asymmetric_private_key channel_write_private_key;
//            if(!walker.get_channel_write_certificate(
//                channel_id,
//                channel_name,
//                channel_read_cert,
//                channel_read_private_key,
//                channel_write_cert,
//                channel_write_private_key)){
//              throw std::runtime_error("Unable to get channel write certificate");
//            }
//            vds::user_channel channel(channel_id, channel_name, channel_read_cert, channel_write_cert);
//
//            sp.get<vds::logger>()->trace("MOCK", sp, "Allow write channel %s(%s). Cert %s",
//                                         channel_name.c_str(),
//                                         channel_id.str().c_str(),
//                                         vds::cert_control::get_id(channel_write_cert).str().c_str());
//
//            vds::asymmetric_private_key device_private_key;
//            auto device_user = user_mng->get_current_device(
//                sp,
//                device_private_key);
//
//            vds::transactions::transaction_block_builder log;
//
//            channel.add_reader(
//                log,
//                device_user,
//                root_user,
//                root_user_private_key,
//                channel_read_private_key);
//
//            log.save(
//                sp,
//                t,
//                [sp, user_mng](const vds::guid & channel_id,
//                               vds::certificate & read_cert,
//                               vds::certificate & write_cert,
//                               vds::asymmetric_private_key & write_private_key){
//                  auto channel = user_mng->get_channel(sp, channel_id);
//                  if(!channel){
//                    throw std::runtime_error("Invalid channel");
//                  }
//                  read_cert = channel.read_cert();
//                  write_cert = channel.write_cert();
//                  write_private_key = user_mng->get_channel_write_key(sp, channel_id);
//                });
//
//            return true;
//          })
//      .execute([&b, &error](const std::shared_ptr<std::exception> & ex){
//        if(ex){
//          error = ex;
//        }
//        b.set();
//      });
//
//  b.wait();
//  if(error){
//    throw std::runtime_error(error->what());
//  }
}

vds::const_data_buffer vds_mock::upload_file(
	size_t client_index,
	const vds::const_data_buffer &channel_id,
	const std::string &name,
  const std::string &mimetype,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & input_stream) {

  auto result = std::make_shared<vds::const_data_buffer>();
  auto sp = this->servers_[client_index]->get_service_provider().create_scope(__FUNCTION__);

  vds::mt_service::enable_async(sp);
  auto user_mng = std::make_shared<vds::user_manager>();
  sp.get<vds::dht::network::client>()->restore(
    sp,
    vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_))
    .then([sp, this, user_mng](const vds::const_data_buffer & crypted_private_key)->vds::async_task<> {
    auto user_private_key = vds::asymmetric_private_key::parse_der(
      vds::symmetric_decrypt::decrypt(
        vds::symmetric_key::from_password(this->root_password_),
        crypted_private_key), std::string());
      return sp.get<vds::db_model>()->async_transaction(
      sp,
      [this, sp, user_mng, user_private_key](vds::database_transaction & t) -> bool {

      user_mng->load(
        sp,
        t,
        vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_),
        user_private_key);

      return true;
    });
  })
  .then([sp, name, channel_id, mimetype, input_stream, user_mng]() {

    return sp.get<vds::file_manager::file_operations>()->upload_file(
        sp,
        user_mng,
        channel_id,
        "test message",
        name,
        mimetype,
        input_stream);
  })
      .then([result](const vds::const_data_buffer & file_hash){
        *result = file_hash;
      })
      .wait();
  return *result;
}

vds::async_task<
    std::string /*content_type*/,
    size_t /*body_size*/,
    std::shared_ptr<vds::continuous_buffer<uint8_t>> /*output_stream*/>
vds_mock::download_data(
	size_t client_index,
	const vds::const_data_buffer &channel_id,
	const std::string &name,
	const vds::const_data_buffer & file_hash) {
  auto sp = this->servers_[client_index]->get_service_provider().create_scope(__FUNCTION__);
  vds::mt_service::enable_async(sp);

  auto user_mng = std::make_shared<vds::user_manager>();
  return sp.get<vds::dht::network::client>()->restore(
          sp,
          vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_))
      .then([sp, this, channel_id, name, file_hash, user_mng](
          const vds::const_data_buffer &crypted_private_key) -> vds::async_task<> {
        auto user_private_key = vds::asymmetric_private_key::parse_der(
            vds::symmetric_decrypt::decrypt(
                vds::symmetric_key::from_password(this->root_password_),
                crypted_private_key), std::string());

        return sp.get<vds::db_model>()->async_transaction(
            sp,
            [this, sp, user_mng, name, user_private_key, channel_id, file_hash](vds::database_transaction &t) {

              user_mng->load(
                  sp,
                  t,
                  vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_),
                  user_private_key);

              return true;
            });
      })
      .then(
          [sp, name, channel_id, file_hash, user_mng]() -> vds::async_task<std::string, size_t, std::shared_ptr<vds::continuous_buffer<uint8_t>>> {


            return sp.get<vds::file_manager::file_operations>()->download_file(
                sp,
                user_mng,
                channel_id,
                file_hash).then(
                [](const vds::file_manager::file_operations::download_result_t &result) -> vds::async_task<std::string, size_t, std::shared_ptr<vds::continuous_buffer<uint8_t>>> {
                  return vds::async_task<std::string, size_t, std::shared_ptr<vds::continuous_buffer<uint8_t>>>::result(
                      result.mime_type,
                      result.size,
                      result.output_stream);
                });
          });
}

vds::user_channel vds_mock::create_channel(int index, const std::string &name) {
  vds::user_channel result;
  auto sp = this->servers_[index]->get_service_provider().create_scope(__FUNCTION__);
  vds::mt_service::enable_async(sp);

  auto user_mng = std::make_shared<vds::user_manager>();
  sp.get<vds::dht::network::client>()->restore(
    sp,
    vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_))
    .then([sp, this, user_mng, name, &result](const vds::const_data_buffer & crypted_private_key)->vds::async_task<> {
      auto user_private_key = vds::asymmetric_private_key::parse_der(
          vds::symmetric_decrypt::decrypt(
              vds::symmetric_key::from_password(this->root_password_),
              crypted_private_key), std::string());
      return sp.get<vds::db_model>()->async_transaction(
          sp,
          [this, sp, user_mng, &result, name, user_private_key](vds::database_transaction &t) -> bool {

            user_mng->load(
                sp,
                t,
                vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_),
                user_private_key);

            auto read_private_key = vds::asymmetric_private_key::generate(
                vds::asymmetric_crypto::rsa4096());
            auto write_private_key = vds::asymmetric_private_key::generate(
                vds::asymmetric_crypto::rsa4096());

            vds::transactions::transaction_block_builder log(sp, t);
            vds::asymmetric_private_key channel_read_private_key;
            vds::asymmetric_private_key channel_write_private_key;
            result = user_mng->create_channel(sp, log, t, vds::dht::dht_object_id::generate_random_id(),
                                              vds::user_channel::channel_type_t::personal_channel,
                                              name,
                                              channel_read_private_key, channel_write_private_key);

            log.save(
                sp,
                t,
                user_mng->get_current_user().user_certificate(),
                user_mng->get_current_user_private_key());

            return true;
          });
    }).wait();

  return result;
}

vds::service_provider vds_mock::get_sp(int client_index) {
  return this->servers_[client_index]->get_service_provider();
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
}

void mock_server::init_root(int index, int udp_port, const std::string& root_password)
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
        .reset(sp, "root", root_password)
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
  this->server_.start_network(this->sp_, this->udp_port_)
      .execute([&b, &error](const std::shared_ptr<std::exception> & ex){
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

void mock_server::init(
    int index,
    int udp_port,
    const std::string &user_login,
    const std::string &user_password) {
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
      .start_network(sp, udp_port)
      .then([sp, user_login, user_password]() ->vds::async_task<> {

        auto user_mng = std::make_shared<vds::user_manager>();
        return sp.get<vds::dht::network::client>()->restore(
                sp,
                vds::dht::dht_object_id::user_credentials_to_key(user_login, user_password))
            .then([sp, user_mng, user_login, user_password](const vds::const_data_buffer & crypted_private_key)->vds::async_task<> {
              auto user_private_key = vds::asymmetric_private_key::parse_der(
                  vds::symmetric_decrypt::decrypt(
                      vds::symmetric_key::from_password(user_password),
                      crypted_private_key), std::string());

              return sp.get<vds::db_model>()->async_transaction(sp, [sp, user_mng, user_login, user_password, user_private_key](
                  vds::database_transaction &t) {
                user_mng->load(
                    sp,
                    t,
                    vds::dht::dht_object_id::user_credentials_to_key(user_login, user_password),
                    user_private_key);
              });
            });

      })
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
    throw std::runtime_error(error->what());
  }

}
