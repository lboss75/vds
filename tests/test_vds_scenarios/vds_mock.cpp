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
#include "device_config_dbo.h"

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
    std::unique_ptr<mock_server> server(new mock_server(i, first_port + i));
    try {
      std::cout << "Starring server " << i << "\n";
      server->start();
      if (0 == i) {
        std::cout << "Initing root\n";
        server->init_root(this->root_login_, this->root_password_);
      }

      //server->allocate_storage(this->root_login_, this->root_password_);
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

static void print_value(const std::string & value, int width) {
  std::cout << value;
  for(int i = value.length(); i < width; ++i) {
    std::cout << ' ';
  }
}

static void print_table(const std::list<std::tuple<std::string, std::map<std::string, std::string>>> & table) {
  std::size_t first_column_width = 0U;
  std::map<std::string, std::size_t> column_witdhs;

  //Calculate
  for(auto row : table) {
    if(first_column_width < std::get<0>(row).length()) {
      first_column_width = std::get<0>(row).length();
    }
    
    for(auto column : std::get<1>(row)) {
      if (column_witdhs[column.first] < column.first.length()) {
        column_witdhs[column.first] = column.first.length();
      }

      if(column_witdhs[column.first] < column.second.length()) {
        column_witdhs[column.first] = column.second.length();
      }
    }
  }
  //Out
  print_value(std::string(), first_column_width);

  for (auto column : column_witdhs) {
    std::cout << '|';
    print_value(column.first, column.second);
  }
  std::cout << '\n';

  for (auto row : table) {
    print_value(std::get<0>(row), first_column_width);

    for (auto column : column_witdhs) {
      std::cout << '|';
      print_value(std::get<1>(row)[column.first], column.second);
    }
    std::cout << '\n';
  }
}


bool vds_mock::dump_statistic(std::vector<vds::server_statistic>& statistics) {
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

  std::map<int, std::list<vds::const_data_buffer>> order_no;
  std::map<vds::const_data_buffer, std::map<std::string, std::string>> states;
  for(std::size_t i = 0; i < statistics.size(); ++i) {
    auto istr = std::to_string(i);
    for (auto p : statistics[i].sync_statistic_.unknown_) {
      states[p][istr] = "?";
    }

    for(auto leaf : statistics[i].sync_statistic_.leafs_) {

      auto & p = order_no[leaf.order_no];
      if(p.end() == std::find(p.begin(), p.end(), leaf.id)) {
        p.push_back(leaf.id);
      }
      switch ((vds::orm::transaction_log_record_dbo::state_t)leaf.state) {
      case vds::orm::transaction_log_record_dbo::state_t::processed:
        states[leaf.id][istr] = "+";
        break;
      case vds::orm::transaction_log_record_dbo::state_t::leaf:
        states[leaf.id][istr] = "V";
        break;
      case vds::orm::transaction_log_record_dbo::state_t::invalid:
        states[leaf.id][istr] = "X";
        break;
      }
    }
  }

  std::list<std::tuple<std::string, std::map<std::string, std::string>>> table;
  for (auto p : order_no) {
    for (auto record : p.second) {
      table.push_back(std::make_tuple(
        std::to_string(p.first) + "." + vds::base64::from_bytes(record),
        states[record]));
    }
  }
  print_table(table);

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
    
  //////////////////////////////////////////////////////////////////////
  table.clear();
  std::cout << "Replicas:\n";
  for (std::size_t i = 0; i < statistics.size(); ++i) {
    for (auto chunk : statistics[i].sync_statistic_.chunks_) {
      std::map<std::string, std::string> * prow = nullptr;
      for (auto & row : table) {
        if (std::get<0>(row) == vds::base64::from_bytes(chunk)) {
          prow = &std::get<1>(row);
          break;
        }
      }
      if (prow == nullptr) {
        table.push_back(std::make_tuple(vds::base64::from_bytes(chunk), std::map<std::string, std::string>()));
        prow = &std::get<1>(*table.rbegin());
      }

      (*prow)[std::to_string(i)] = "*";
    }

    for (auto chunk : statistics[i].sync_statistic_.chunk_replicas_) {

      std::map<std::string, std::string> * prow = nullptr;
      for(auto & row : table) {
        if(std::get<0>(row) == vds::base64::from_bytes(chunk.first)) {
          prow = &std::get<1>(row);
          break;
        }
      }
      if(prow == nullptr) {
        table.push_back(std::make_tuple(vds::base64::from_bytes(chunk.first), std::map<std::string, std::string>()));
        prow = &std::get<1>(*table.rbegin());
      }

      auto & val = (*prow)[std::to_string(i)];

      for (auto replica : chunk.second) {
        if (val.empty()) {
          val = std::to_string(replica);
        }
        else {
          val += ',';
          val += std::to_string(replica);
        }
      }
    }
  }
  print_table(table);

  return is_good;
}

void vds_mock::sync_wait()
{
  std::cout << "Waiting to synchronize...\n";
  for(int i = 0; i < 1000; ++i){

    std::vector<vds::server_statistic> statistics;
    if (this->dump_statistic(statistics)) {
      return;
    }

    if(i > 5){
      int index = 0;
      for(auto & p : statistics) {
        std::cout << "[" << index++ << "]:" << p.route_statistic_.serialize()->str() << "\n";
      }
    }
    //

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
//                root_user.object_id(),
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
    std::cout << "Got user private key\n";
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
      name,
      mimetype,
      input_stream);
  })
  .then([sp, name, channel_id, mimetype, input_stream, user_mng, result](const vds::transactions::user_message_transaction::file_info_t & file_info) {
    *result = file_info.file_id;
    std::list<vds::transactions::user_message_transaction::file_info_t> files {file_info};
      return sp.get<vds::file_manager::file_operations>()->create_message(
        sp,
        user_mng,
        channel_id,
        "test message",
        files);
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
    std::cout << "Got user private key\n";
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
          [this, sp, user_mng, user_private_key](vds::database_transaction &t) -> bool {

            user_mng->load(
                sp,
                t,
                vds::dht::dht_object_id::user_credentials_to_key(this->root_login_, this->root_password_),
                user_private_key);


            return true;
          });
    })
    .then([sp, user_mng, name]() {
      return user_mng->create_channel(sp, name);
    }).then([&result](const vds::user_channel & channel) {
        result = channel;
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

void mock_server::init_root(
  const std::string &root_user_name,
  const std::string &root_password) const {
  vds::cert_control::private_info_t private_info;
  private_info.genereate_all();
  vds::cert_control::genereate_all(root_user_name, root_password, private_info);

  auto user_mng = std::make_shared<vds::user_manager>();

  const auto sp = this->get_service_provider().create_scope(__FUNCTION__);
  vds::mt_service::enable_async(sp);

  user_mng->reset(sp, root_user_name, root_password, private_info);
}

//
//void mock_server::allocate_storage(const std::string& root_login, const std::string& root_password) {
//  this->login(root_login, root_password,[](const vds::service_provider & sp, const std::shared_ptr<vds::user_manager> & user_mng) {
//    return sp.get<db_model>()->async_transaction(sp, [sp, user_mng](database_transaction & t) {
//      auto client = sp.get<dht::network::client>();
//      auto current_node = client->current_node_id();
//      foldername fl(foldername(persistence::current_user(sp), ".vds"), "storage");
//      fl.create();
//
//
//      vds::orm::device_config_dbo t1;
//      t.execute(
//        t1.insert(
//          t1.node_id = current_node,
//          t1.local_path = fl.full_name(),
//          t1.owner_id = user_mng->get_current_user().user_certificate().subject(),
//          t1.name = device_name,
//          t1.reserved_size = reserved_size * 1024 * 1024 * 1024));
//    });
//
//  });
//}

void mock_server::login(
  const std::string& root_login,
  const std::string& root_password,
  const std::function<void(const vds::service_provider & sp, const std::shared_ptr<vds::user_manager> & user_mng)> & callback) {

auto sp = this->get_service_provider().create_scope(__FUNCTION__);
vds::mt_service::enable_async(sp);

auto user_mng = std::make_shared<vds::user_manager>();
sp.get<vds::dht::network::client>()->restore(
  sp,
  vds::dht::dht_object_id::user_credentials_to_key(root_login, root_password))
  .then([sp, this, user_mng, root_login, root_password](
    const vds::const_data_buffer &crypted_private_key) -> vds::async_task<> {
  std::cout << "Got user private key\n";
  auto user_private_key = vds::asymmetric_private_key::parse_der(
    vds::symmetric_decrypt::decrypt(
      vds::symmetric_key::from_password(root_password),
      crypted_private_key), std::string());

  return sp.get<vds::db_model>()->async_transaction(
    sp,
    [sp, user_mng, user_private_key, root_login, root_password](vds::database_transaction &t) {

    user_mng->load(
      sp,
      t,
      vds::dht::dht_object_id::user_credentials_to_key(root_login, root_password),
      user_private_key);

  });
})
.then(
  [sp, user_mng, callback]() {


  return callback(
    sp,
    user_mng);
  }).wait();

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
