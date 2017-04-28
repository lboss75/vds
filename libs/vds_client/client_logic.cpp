/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_logic.h"

vds::client_logic::client_logic(
  const service_provider & sp,
  const std::string & server_address,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
  : sp_(sp),
  log_(sp, "VDS Client logic"),
  server_address_(server_address),
  client_certificate_(client_certificate),
  client_private_key_(client_private_key),
  filter_last_index_(0),
  connected_(0),
  timer_task_(std::bind(&client_logic::process_timer_tasks, this)),
  outgoing_queue_(sp),
  messages_sent_(false)
{  
}

vds::client_logic::~client_logic()
{
}

void vds::client_logic::start()
{
  url_parser::parse_addresses(this->server_address_,
    [this](const std::string & protocol, const std::string & address)->bool {
    if ("https" == protocol) {
      auto url = url_parser::parse_network_address(address);
      if (protocol == url.protocol) {
        this->connection_queue_.push_back(
          std::unique_ptr<client_connection<client_logic>>(
            new client_connection<client_logic>(
              this->sp_,
              this,
              url.server,
              std::atoi(url.port.c_str()),
              this->client_certificate_,
              this->client_private_key_)));
      }
    }
    return true;
  });

  this->process_timer_tasks();
}

void vds::client_logic::stop()
{
  if (std::future_status::ready != this->update_connection_pool_feature_.wait_for(std::chrono::seconds(0))) {
    this->update_connection_pool_feature_.get();
  }
}

void vds::client_logic::connection_closed(client_connection<client_logic>& connection)
{
  sp.get<logger>().info(sp, "Connection %s:%d has been closed", connection.address().c_str(), connection.port());

  this->connection_mutex_.lock();
  this->connected_--;
  this->connection_mutex_.unlock();

  this->process_timer_tasks();
}

void vds::client_logic::connection_error(client_connection<client_logic>& connection, std::exception_ptr ex)
{
  sp.get<logger>().info(sp, "Connection %s:%d error %s", connection.address().c_str(), connection.port(), exception_what(ex).c_str());

  this->connection_mutex_.lock();
  this->connected_--;
  this->connection_mutex_.unlock();

  this->process_timer_tasks();
}

void vds::client_logic::process_response(client_connection<client_logic>& connection, const json_value * response)
{
  auto tasks = dynamic_cast<const json_array *>(response);
  if (nullptr != tasks) {
    for (size_t i = 0; i < tasks->size(); ++i) {
      auto task = dynamic_cast<const json_object *>(tasks->get(i));

      if (nullptr != task) {
        std::string request_id;
        if(task->get_property("$r", request_id)){
          std::lock_guard<std::mutex> lock(this->requests_mutex_);
          auto p = this->requests_.find(request_id);
          if(this->requests_.end() != p){
            imt_service::async(this->sp_,
              [
                done = p->second.done,
                result = task->clone().release()](){
              done(std::unique_ptr<json_value>(result).get());
            });
            this->requests_.remove(request_id);            
          }
        }
        
        std::string task_type;
        if (task->get_property("$t", task_type, false)) {
          std::unique_lock<std::mutex> lock(this->filters_mutex_);
          for (auto & f : this->filters_[task_type]) {
            f.second(task);
          }
        }
      }
    }
  }
}

void vds::client_logic::cancel_request(const std::string& request_id)
{
  std::lock_guard<std::mutex> lock(this->requests_mutex_);
  auto p = this->requests_.find(request_id);
  if (this->requests_.end() != p) {
    imt_service::async(this->sp_,
      [on_error = p->second.on_error](){
      on_error(std::make_exception_ptr(std::runtime_error("Timeout")));
    });
  }

  this->requests_.remove(request_id);
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

void vds::client_logic::get_commands(vds::client_connection<vds::client_logic> & connection)
{
  this->sp_.get<imt_service>().async(
    [this, &connection](){ 
    if (this->outgoing_queue_.get(connection)) {
      this->messages_sent_ = true;
    }});
}

void vds::client_logic::add_task(const std::string & message)
{
  this->outgoing_queue_.push(message);
}

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
void vds::client_logic::process_timer_tasks()
{
  if (!this->connection_queue_.empty()) {
    if (
      !this->update_connection_pool_feature_.valid()
      || std::future_status::ready == this->update_connection_pool_feature_.wait_for(std::chrono::seconds(0))) {
      this->update_connection_pool_feature_ = std::async(
        std::launch::async,
        [this]() {

        std::chrono::time_point<std::chrono::system_clock> border
          = std::chrono::system_clock::now() - std::chrono::seconds(60);

        size_t try_count = 0;
        this->connection_mutex_.lock();
        while (
          !this->sp_.get_shutdown_event().is_shuting_down()
          && this->connected_ < MAX_CONNECTIONS
          && try_count++ < MAX_CONNECTIONS
          && this->connected_ < this->connection_queue_.size()) {


          auto index = std::rand() % this->connection_queue_.size();
          auto& connection = this->connection_queue_[index];
          if (
            client_connection<client_logic>::NONE == connection->state()
            || (
              client_connection<client_logic>::CONNECT_ERROR == connection->state()
              && border > connection->connection_end()
              )
            ) {

            this->connection_mutex_.unlock();
            connection->connect();
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
      this->add_task(r->second.task);
    }
  }
  else {
    this->messages_sent_ = false;
  }
  
  this->sp_.get<itask_manager>().wait_for(std::chrono::seconds(5)) += this->timer_task_;
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

vds::async_task<const std::string& /*version_id*/> vds::client_logic::put_file(
  const std::string & user_login,
  const std::string & name,
  const const_data_buffer & data)
{  
  foldername tmp(persistence::current_user(this->sp_), "tmp");
  tmp.create();

  auto version_id = guid::new_guid().str();
  filename tmpfile(tmp, version_id);
  file f(tmpfile, file::create_new);
  f.write(data.data(), data.size());
  f.close();

  return this->send_request<client_messages::put_file_message_response>(
    client_messages::put_file_message(
      user_login,
      name,
      tmpfile).serialize())
    .then([](
      const std::function<void(const std::string& /*version_id*/)> & done,
      const error_handler & on_error,
      const client_messages::put_file_message_response & response) {
    done(response.version_id());
  });
}

vds::async_task<const vds::const_data_buffer & /*datagram*/> vds::client_logic::download_file(
  const std::string & user_login,
  const std::string & name)
{
  auto request_id = guid::new_guid().str();
  std::string error;
  std::string datagram;

  return this->send_request<client_messages::get_file_message_response>(
    client_messages::get_file_message_request(
      user_login,
      name).serialize())
    .then([](
      const std::function<void(const vds::const_data_buffer & /*datagram*/)> & done,
      const error_handler & on_error,
      const client_messages::get_file_message_response & response) {
    
    done(file::read_all(response.tmp_file()));
  });
}
