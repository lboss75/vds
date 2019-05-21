/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "test_log.h"
#include "vds_mock.h"
#include "test_config.h"
#include "private/server_p.h"
#include "file_operations.h"
#include "db_model.h"
#include "user_manager.h"
#include "dht_object_id.h"

vds_mock::vds_mock()
{
}

vds_mock::~vds_mock()
{
  this->stop();
}

void vds_mock::start(size_t server_count)
{
  GET_EXPECTED_THROW(current_process, vds::filename::current_process());
  const auto contains_folder = current_process.contains_folder();
  const auto servers_folder = vds::foldername(contains_folder, "servers");
  CHECK_EXPECTED_THROW(servers_folder.delete_folder(true));
  
  const auto clients_folder = vds::foldername(contains_folder, "clients");
  CHECK_EXPECTED_THROW(clients_folder.delete_folder(true));

  this->root_login_ = "root";
  this->root_password_ = generate_password();
  const auto first_port = 8050;

  for (size_t i = 0; i < server_count; ++i) {
    std::unique_ptr<mock_server> server(new mock_server(i, first_port + i));
    try {
      std::cout << "Starring server " << i << "\n";
      server->start();
      if (5 == i) {
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



bool vds_mock::dump_statistic(std::ostream & logfile, std::vector<vds::server_statistic>& statistics) {
  for (auto & p : this->servers_) {
    GET_EXPECTED_THROW(statistic, p->get_statistic().get());
    statistics.push_back(statistic);
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
  print_table(logfile, table);

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
  std::map<vds::const_data_buffer, std::map<std::size_t, std::string>> objects;
  logfile << "Replicas:\n";
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
      objects[chunk][i] = "*";
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
          objects[chunk.first][i] = std::to_string(replica);
        }
        else {
          val += ',';
          val += std::to_string(replica);
          objects[chunk.first][i] = val;
        }
      }
    }
  }
  print_table(logfile, table);
  //Sync state
  std::map<vds::const_data_buffer, std::size_t> node_id2index;
  for (std::size_t i = 0; i < statistics.size(); ++i) {
    node_id2index[statistics[i].route_statistic_.node_id_] = i;
  }
  for (const auto & object : objects) {
    logfile << "Object replicas:" << vds::base64::from_bytes(object.first) << "\n";

    table.clear();
    std::map<std::string, std::string> columns;
    for (std::size_t i = 0; i < statistics.size(); ++i) {

      const auto p = object.second.find(i);
      if (object.second.end() != p) {
        columns[std::to_string(i)] = p->second;
      }

    }
    table.push_back(std::make_tuple(
      "Real",
      columns));

    for (std::size_t i = 0; i < statistics.size(); ++i) {
      columns.clear();

      const auto p1 = statistics[i].sync_statistic_.sync_states_.find(object.first);
      if (statistics[i].sync_statistic_.sync_states_.end() != p1) {
        columns["S"] = p1->second.node_state_;

        for (const auto & sync_member : p1->second.members_) {
          std::string val;
          for(const auto & item : sync_member.second.replicas_) {
            if(val.empty()) {
              val = std::to_string(item);
            }
            else {
              val += ',';
              val += std::to_string(item);
            }
          }
          columns[std::to_string(node_id2index[sync_member.first])] = val;
        }

        table.push_back(std::make_tuple(
          std::to_string(i) + "." + vds::base64::from_bytes(statistics[i].route_statistic_.node_id_),
          columns));
      }
    }
    print_table(logfile, table);

    for (std::size_t i = 0; i < statistics.size(); ++i) {
      const auto p1 = statistics[i].sync_statistic_.sync_states_.find(object.first);
      if (statistics[i].sync_statistic_.sync_states_.end() != p1 && !p1->second.messages_.empty()) {
        table.clear();
        for (const auto & message : p1->second.messages_) {
          std::map<std::string, std::string> columns;
          columns["T"] = message.second.message_type_;
          columns["M"] = std::to_string(node_id2index[message.second.member_node_]) + "." + vds::base64::from_bytes(message.second.member_node_);
          columns["R"] = std::to_string(message.second.replica_);
          columns["S"] = std::to_string(node_id2index[message.second.source_node_]) + "." + vds::base64::from_bytes(message.second.source_node_);
          columns["I"] = std::to_string(message.second.source_index_);
          table.push_back(std::make_tuple(
            std::to_string(message.first),
            columns));
        }
        print_table(logfile, table);
      }
    }
  }
  //Network
  table.clear();
  logfile << "Route:\n";
  for (std::size_t i = 0; i < statistics.size(); ++i) {
    std::map<std::string, std::string> columns;

    for(const auto & item : statistics[i].route_statistic_.items_){
      if(item.pinged_ >= 8) {
        continue;
      }

      auto index = std::to_string(node_id2index.at(item.node_id_));
      auto p = columns.find(index);
      if(p == columns.end()){
        columns[index] = std::to_string(item.hops_);
      }
      else if(item.hops_ < atoi(p->second.c_str())){
        p->second = std::to_string(item.hops_);
      }
    }

    table.push_back(
        std::make_tuple(
            std::to_string(i) + "." + vds::base64::from_bytes(statistics[i].route_statistic_.node_id_),
            columns));
  }
  print_table(logfile, table);

  return is_good;
}

void vds_mock::sync_wait()
{
  std::ofstream logfile("test.log", std::ofstream::app);

  std::cout << "Waiting to synchronize...\n";
  for(int i = 0; i < 1000; ++i){

    std::vector<vds::server_statistic> statistics;
    if (this->dump_statistic(logfile, statistics)) {
      logfile.flush();
      return;
    }

    /*
    if(i > 5){
      int index = 0;
      for(auto & p : statistics) {
        std::cout << "[" << index++ << "]:" << p.route_statistic_.serialize()->str() << "\n";
      }
    }
    */
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
//              return vds::make_unexpected<std::runtime_error>("Unable to get channel write certificate");
//            }
//			vds::user_channel channel(channel_id, channel_name, channel_read_cert, channel_write_cert);
//
//			sp->get<vds::logger>()->trace("MOCK", sp, "Allow write channel %s(%s). Cert %s",
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
//                    return vds::make_unexpected<std::runtime_error>("Invalid channel");
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
//    return vds::make_unexpected<std::runtime_error>(error->what());
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
//              return vds::make_unexpected<std::runtime_error>("Unable to get channel write certificate");
//            }
//            vds::user_channel channel(channel_id, channel_name, channel_read_cert, channel_write_cert);
//
//            sp->get<vds::logger>()->trace("MOCK", sp, "Allow write channel %s(%s). Cert %s",
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
//                    return vds::make_unexpected<std::runtime_error>("Invalid channel");
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
//    return vds::make_unexpected<std::runtime_error>(error->what());
//  }
}

vds::const_data_buffer vds_mock::upload_file(
  size_t client_index,
  const vds::const_data_buffer &channel_id,
  const std::string &name,
  const std::string &mimetype,
  const std::shared_ptr<vds::stream_input_async<uint8_t>>& input_stream) {

  auto sp = this->servers_[client_index]->get_service_provider();

  auto user_mng = std::make_shared<vds::user_manager>(sp);

  CHECK_EXPECTED_THROW(sp->get<vds::db_model>()->async_transaction(
    [this, user_mng](vds::database_transaction & t) -> vds::expected<void> {

    return user_mng->load(
      t,
      this->root_login_,
      this->root_password_);
  }).get());

  std::list<vds::transactions::user_message_transaction::file_info_t> files;
  GET_EXPECTED_THROW(stream, sp->get<vds::file_manager::file_operations>()->upload_file(
    name,
    mimetype,
    vds::const_data_buffer(),
    [&files](vds::transactions::user_message_transaction::file_info_t && file_info) -> vds::async_task<vds::expected<void>> {
    files.push_back(file_info);
    return vds::expected<void>();
  }).get());

  
  CHECK_EXPECTED_THROW(input_stream->copy_to(stream).get());

  auto message = std::make_shared<vds::json_object>();
  message->add_property("$type", "SimpleMessage");
  message->add_property("message", "test message");

  CHECK_EXPECTED_THROW(sp->get<vds::file_manager::file_operations>()->create_message(
    user_mng,
    channel_id,
    message,
    files).get());

  return files.begin()->file_id;
}

vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
vds_mock::download_data(
  size_t client_index,
  const vds::const_data_buffer &channel_id,
  const std::string &name,
  const vds::const_data_buffer & file_hash,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream) {
  auto sp = this->servers_[client_index]->get_service_provider();

  auto user_mng = std::make_shared<vds::user_manager>(sp);

  CHECK_EXPECTED_ASYNC(co_await sp->get<vds::db_model>()->async_transaction(
    [this, user_mng, channel_id, file_hash](vds::database_transaction &t) -> vds::expected<void>{

    return user_mng->load(
      t,
      this->root_login_,
      this->root_password_);
  }));


  co_return co_await sp->get<vds::file_manager::file_operations>()->download_file(
    user_mng,
    channel_id,
    name,
    file_hash,
    output_stream);
}

vds::user_channel vds_mock::create_channel(int index, const std::string & channel_type, const std::string &name) {

  vds::user_channel result;

  auto sp = this->servers_[index]->get_service_provider();

  auto user_mng = std::make_shared<vds::user_manager>(sp);

  CHECK_EXPECTED_THROW(sp->get<vds::db_model>()->async_transaction(
    [this, user_mng](vds::database_transaction &t) -> vds::expected<void> {

    return user_mng->load(
      t,
      this->root_login_,
      this->root_password_);
  }).get());

  GET_EXPECTED_THROW(result_channel, user_mng->create_channel(channel_type, name).get());
  return result_channel;
}

const vds::service_provider * vds_mock::get_sp(int client_index) {
  return this->servers_[client_index]->get_service_provider();
}

mock_server::mock_server(int index, int udp_port)
  : index_(index),
  tcp_port_(udp_port),
  udp_port_(udp_port),
  sp_(nullptr),
  logger_(
    test_config::instance().log_level(),
    test_config::instance().modules())
{
}

void mock_server::init_root(
  const std::string &root_user_name,
  const std::string &root_password) const {
  vds::cert_control::private_info_t private_info;
  CHECK_EXPECTED_THROW(private_info.genereate_all());
  CHECK_EXPECTED_THROW(vds::cert_control::genereate_all(root_user_name, root_password, private_info));

  const auto sp = this->get_service_provider();
  auto user_mng = std::make_shared<vds::user_manager>(sp);
  CHECK_EXPECTED_THROW(user_mng->reset(root_user_name, root_password, private_info));
}

//
//void mock_server::allocate_storage(const std::string& root_login, const std::string& root_password) {
//  this->login(root_login, root_password,[]( const std::shared_ptr<vds::user_manager> & user_mng) {
//    return sp->get<db_model>()->async_transaction(sp, [sp, user_mng](database_transaction & t) {
//      auto client = sp->get<dht::network::client>();
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
  const std::function<void( const std::shared_ptr<vds::user_manager> & user_mng)> & callback) {

auto sp = this->get_service_provider();
auto user_mng = std::make_shared<vds::user_manager>(sp);

  CHECK_EXPECTED_THROW(sp->get<vds::db_model>()->async_transaction(
    [user_mng, root_login, root_password](vds::database_transaction &t) -> vds::expected<void> {

    return user_mng->load(
      t,
      root_login,
      root_password);

  }).get());

  callback(
    user_mng);

}

void mock_server::start()
{
  GET_EXPECTED_THROW(current_process, vds::filename::current_process());
  auto folder = vds::foldername(
    vds::foldername(current_process.contains_folder(), "servers"),
    std::to_string(this->index_));
  CHECK_EXPECTED_THROW(folder.create());
  
 
  this->registrator_.add(this->mt_service_);
  this->registrator_.add(this->logger_);
  this->registrator_.add(this->task_manager_);
  this->registrator_.add(this->network_service_);
  this->registrator_.add(this->crypto_service_);
  this->registrator_.add(this->server_);

  //this->connection_manager_.set_addresses("udp://127.0.0.1:" + std::to_string(8050 + this->index_));
  this->registrator_.current_user(folder);
  this->registrator_.local_machine(folder);

  GET_EXPECTED_VALUE_THROW(this->sp_, this->registrator_.build());
  CHECK_EXPECTED_THROW(this->registrator_.start());

  CHECK_EXPECTED_THROW(this->server_.start_network(this->udp_port_, true).get());
  this->sp_->get<vds::dht::network::client>()->update_wellknown_connection_enabled(false);
}

void mock_server::stop()
{
  CHECK_EXPECTED_THROW(this->registrator_.shutdown());
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

  GET_EXPECTED_THROW(current_process, vds::filename::current_process());
  auto folder = vds::foldername(
    vds::foldername(current_process.contains_folder(), "servers"),
    std::to_string(index));
  CHECK_EXPECTED_THROW(folder.delete_folder(true));
  CHECK_EXPECTED_THROW(folder.create());

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(task_manager);
  registrator.add(crypto_service);
  registrator.add(network_service);
  registrator.add(server);

  registrator.current_user(folder);
  registrator.local_machine(folder);

  GET_EXPECTED_THROW(sp, registrator.build());
  CHECK_EXPECTED_THROW(registrator.start());

  CHECK_EXPECTED_THROW(server.start_network(udp_port, true).get());

  auto user_mng = std::make_shared<vds::user_manager>(sp);

  CHECK_EXPECTED_THROW(sp->get<vds::db_model>()->async_transaction([user_mng, user_login, user_password](
    vds::database_transaction &t) {
    return user_mng->load(
      t,
      user_login,
      user_password);
  }).get());


  CHECK_EXPECTED_THROW(registrator.shutdown());
}
