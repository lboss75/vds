/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_logic.h"
#include "http_request.h"

vds::client_logic::client_logic()
: connected_(0),  
  messages_sent_(false),
  process_timer_("client process timer")
{
}

vds::client_logic::~client_logic()
{
}

void vds::client_logic::start(
  const service_provider & sp,
  const std::string & server_address,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
{
  auto scope = sp.create_scope("Client login");
  imt_service::enable_async(scope);

  this->server_address_ = server_address;
  this->client_certificate_ = client_certificate;
  this->client_private_key_ = client_private_key;

  url_parser::parse_addresses(this->server_address_,
    [this, scope](const std::string & protocol, const std::string & address)->bool {
    if ("https" == protocol) {
      auto url = url_parser::parse_network_address(address);
      if (protocol == url.protocol) {
        std::unique_ptr<client_connection> connection(
          new client_connection(
            url.server,
            std::atoi(url.port.c_str()),
            this->client_certificate_,
            this->client_private_key_));
        this->continue_read_connection(scope, connection.get(), std::make_shared<std::shared_ptr<http_message>>());
        this->connection_queue_.push_back(std::move(connection));
      }
    }
    return true;
  });

  this->process_timer_.start(
    scope,
    std::chrono::seconds(5),
    [this, scope](){
      return this->process_timer_tasks(scope);
  });
}

void vds::client_logic::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);

  this->connection_mutex_.lock();
  for (auto & connection : this->connection_queue_) {
    if (client_connection::STATE::CONNECTED == connection->state()) {
      auto scope = sp.create_scope("Call HTTP Client API");
      imt_service::enable_async(scope);

      barrier b;
      connection->outgoing_stream()->write_all_async(scope, nullptr, 0)
        .wait(
          [&b](const service_provider & sp) {b.set(); },
          [&b](const service_provider & sp, const std::shared_ptr<std::exception> & ex) { b.set(); sp.unhandled_exception(ex); },
          scope);
      b.wait();
    }
    connection->stop(sp);
  }
  this->connection_mutex_.unlock();


  if (std::future_status::ready != this->update_connection_pool_feature_.wait_for(std::chrono::seconds(0))) {
    this->update_connection_pool_feature_.get();
  }
}

void vds::client_logic::process_response(
  const service_provider & sp,
  //client_connection<client_logic>& connection,
  const std::shared_ptr<json_value> & response)
{
  sp.get<logger>()->trace(sp, "Response %s", response->str().c_str());
  
  auto tasks = std::dynamic_pointer_cast<json_array>(response);
  if (tasks) {
    for (size_t i = 0; i < tasks->size(); ++i) {
      auto task = std::dynamic_pointer_cast<json_object>(tasks->get(i));

      if (task) {
        std::string request_id;
        if(task->get_property("$r", request_id)){
          std::unique_lock<std::mutex> lock(this->requests_mutex_);
          auto p = this->requests_.find(request_id);
          if(this->requests_.end() != p){
            auto item = p->second;
            this->requests_.remove(request_id);
            lock.unlock();

            item->done(sp, task);
          }
        }
      }
    }
  }
}

void vds::client_logic::cancel_request(const service_provider & sp, const std::string& request_id)
{
  std::unique_lock<std::mutex> lock(this->requests_mutex_);
  auto p = this->requests_.find(request_id);
  if (this->requests_.end() != p) {
    auto item = p->second;
    this->requests_.remove(request_id);

    lock.unlock();

    item->on_timeout(sp);
  }
}

/*
void vds::client_logic::node_install(const std::string & login, const std::string & password)
{
  hash password_hash(hash::sha256());
  password_hash.update(password.c_str(), password.length());
  password_hash.final();

  this->install_node_prepare_message_.user_id = "login:" + login;
  this->install_node_prepare_message_.password_hash = base64::from_bytes(password_hash.signature(), password_hash.signature_length());
  this->install_node_prepare_message_.request_id = guid::new_guid_string();

  std::unique_ptr<json_object> request(this->install_node_prepare_message_.serialize());

  json_writer body;
  request->str(body);

  this->query_all(body.str());
}
*/


/*
std::string vds::client_logic::get_messages()
{
  std::string result("[");

  bool bfirst = true;

  std::unique_lock<std::mutex> lock(this->outgoing_queue_mutex_);
  for (auto& message : this->outgoing_queue_) {
    if (bfirst) {
      bfirst = false;
    }
    else {
      result += ',';
    }

    result += message;
  }

  result += ']';

  return result;
}
*/

void vds::client_logic::add_task(const service_provider & sp, const std::shared_ptr<json_value> & message)
{
  std::lock_guard<std::mutex> lock(this->connection_mutex_);
  
  for (auto& connection : this->connection_queue_){
    if (client_connection::STATE::CONNECTED == connection->state()) {
      auto scope = sp.create_scope("Call HTTP Client API");
      imt_service::enable_async(scope);

      connection->outgoing_stream()->write_value_async(scope, message)
      .wait(
        [](const service_provider & sp) {},
        [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) { sp.unhandled_exception(ex); },
        scope);
    }
  }
}

bool vds::client_logic::process_timer_tasks(const service_provider & sp)
{
  if (!this->connection_queue_.empty()) {
    if (
      !this->update_connection_pool_feature_.valid()
      || std::future_status::ready == this->update_connection_pool_feature_.wait_for(std::chrono::seconds(0))) {
      this->update_connection_pool_feature_ = std::async(
        std::launch::async,
        [this, sp]() {

        std::chrono::time_point<std::chrono::steady_clock> border
          = std::chrono::steady_clock::now() - std::chrono::seconds(60);

        size_t try_count = 0;
        this->connection_mutex_.lock();
        while (
          !sp.get_shutdown_event().is_shuting_down()
          && this->connected_ < MAX_CONNECTIONS
          && try_count++ < MAX_CONNECTIONS
          && this->connected_ < this->connection_queue_.size()) {


          auto index = std::rand() % this->connection_queue_.size();
          auto& connection = this->connection_queue_[index];
          if (
            client_connection::STATE::NONE == connection->state()
            || (
              client_connection::STATE::CONNECT_ERROR == connection->state()
              && border > connection->connection_end()
              )
            ) {

            this->connection_mutex_.unlock();
            connection->connect(sp);
            this->connection_mutex_.lock();

            this->connected_++;
          }
        }

        this->connection_mutex_.unlock();
      });
    }
  }

  if (!this->messages_sent_) {
    std::lock_guard<std::mutex> lock(this->requests_mutex_);
    for (decltype(this->requests_)::data_type::const_iterator r = this->requests_.begin(); r != this->requests_.end(); ++r) {
      auto task = r->second->task();
      if (task) {
        this->add_task(sp, task);
      }
    }
  }
  else {
    this->messages_sent_ = false;
  }

  return true;
}

void vds::client_logic::continue_read_connection(
  const service_provider & sp,
  client_connection * connection,
  std::shared_ptr<std::shared_ptr<http_message>> buffer)
{
  connection->incoming_stream()->read_async(sp, buffer.get(), 1)
    .wait([this, connection, buffer](const service_provider & sp, size_t readed) {
      if (0 < readed) {
        auto json_response = std::make_shared<std::shared_ptr<json_value>>();
        dataflow(
          stream_read<continuous_stream<uint8_t>>((*buffer)->body()),
          byte_to_char(),
          json_parser("server response"),
          dataflow_require_once<std::shared_ptr<json_value>>(json_response.get())
        )(
          [this, json_response, connection, buffer](const service_provider & sp) {
            this->process_response(sp, *json_response);
            this->continue_read_connection(sp, connection, buffer);
          },
          [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
          },
          sp);
      }
    },
    [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
    sp);
}

/*
void vds::client_logic::process(client_connection<client_logic>* connection, const install_node_prepared & message)
{
  if (
    this->install_node_prepare_message_.user_id != message.user_id
    && this->install_node_prepare_message_.request_id != message.request_id) {
    return;
  }


}
*/

//vds::async_task<const std::string& /*version_id*/> vds::client_logic::put_file(
//  const service_provider & sp,
//  const std::string & user_login,
//  const std::string & name,
//  const const_data_buffer & meta_info,
//  const filename & tmp_file)
//{  
//  foldername tmp(persistence::current_user(sp), "tmp");
//  tmp.create();
//
//  auto version_id = guid::new_guid().str();
//
//  return this->send_request<client_messages::put_object_message_response>(
//    sp,
//    client_messages::put_object_message(
//      user_login,
//      name,
//      meta_info,
//      tmp_file).serialize())
//    .then([](
//      const std::function<void(const service_provider & sp, const std::string& /*version_id*/)> & done,
//      const error_handler & on_error,
//      const service_provider & sp, 
//      const client_messages::put_object_message_response & response) {
//    done(sp, response.version_id());
//  });
//}
//
//vds::async_task<vds::const_data_buffer /*meta_info*/, vds::filename> vds::client_logic::download_file(
//  const service_provider & sp,
//  const std::string & user_login,
//  const std::string & name)
//{
//  auto request_id = guid::new_guid().str();
//  std::string error;
//  std::string datagram;
//
//  return this->send_request<client_messages::get_file_message_response>(
//    sp,
//    client_messages::get_file_message_request(
//      user_login,
//      name).serialize())
//    .then([](
//      const std::function<void(const service_provider & sp, const_data_buffer meta_info, filename tmp_file)> & done,
//      const error_handler & on_error,
//      const service_provider & sp,
//      const client_messages::get_file_message_response & response) {
//    
//    done(sp, response.meta_info(), response.tmp_file());
//  });
//}

vds::client_logic::request_info::request_info(
  const std::shared_ptr<json_value> & task,
  const std::function<void(const service_provider & sp, const std::shared_ptr<json_value> & response)> & done,
  const error_handler & on_error)
: task_(task),
  done_(done),
  on_error_(on_error),
  is_completed_(false)
{
}

void vds::client_logic::request_info::done(const service_provider & sp, const std::shared_ptr<json_value>& response)
{
  std::lock_guard<std::mutex> task_lock(this->mutex_);
  if (!this->is_completed_) {
    this->is_completed_ = true;
    this->done_(sp, response);
  }
}

void vds::client_logic::request_info::on_timeout(const service_provider & sp)
{
  std::lock_guard<std::mutex> task_lock(this->mutex_);
  if (!this->is_completed_) {
    this->is_completed_ = true;
    this->on_error_(sp, std::make_shared<std::runtime_error>("Timeout"));
  }
}

std::shared_ptr<vds::json_value> vds::client_logic::request_info::task() const
{
  std::lock_guard<std::mutex> task_lock(this->mutex_);

  if (!this->is_completed_) {
    return this->task_;
  }
  else {
    return std::shared_ptr<json_value>();
  }
}
