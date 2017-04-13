/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_client_api.h"
#include "server_json_client_api_p.h"
#include "server.h"
#include "node_manager.h"

vds::server_json_client_api::server_json_client_api(
  const service_provider & sp
)
: impl_(new _server_json_client_api(sp, this))
{
}

vds::server_json_client_api::~server_json_client_api()
{
  delete this->impl_;
}

vds::json_value * vds::server_json_client_api::operator()(
  const service_provider & scope,
  const json_value * request) const
{
  return this->impl_->operator()(scope, request);
}

///////////////////////////////////////////////////////////////////////////////
vds::_server_json_client_api::_server_json_client_api(
  const service_provider & sp,
  server_json_client_api * owner
)
  : log_(sp, "Server JSON Client API"),
  owner_(owner)
{
}

vds::json_value * vds::_server_json_client_api::operator()(
  const service_provider & scope, 
  const json_value * request)
{
  //auto cert = this->tunnel_.get_tunnel_certificate();
  //this->log_.trace("Certificate subject %s", cert.subject().c_str());
  //this->log_.trace("Certificate issuer %s", cert.issuer().c_str());

  std::unique_ptr<json_array> result(new json_array());

  auto request_tasks = dynamic_cast<const json_array *>(request);
  if (nullptr != request_tasks) {
    for (size_t i = 0; i < request_tasks->size(); ++i) {
      auto task = request_tasks->get(i);

      auto task_object = dynamic_cast<const json_object *>(task);
      if (nullptr != task_object) {
        std::string request_id;
        if(task_object->get_property("$r", request_id)){
          bool need_start;
          this->task_mutex_.lock();
          auto p = this->tasks_.find(request_id);
          if(this->tasks_.end() == p){
            need_start = true;
            this->tasks_.set(request_id, task_info());
          }
          else {
            need_start = false;
            if(p->second.result){
              result->add(p->second.result->clone());
            }
          }
          this->task_mutex_.unlock();
          
          if(need_start){
            std::string task_type_name;
            task_object->get_property("$t", task_type_name);
              
            async_task<const json_value *> task;

            if (client_messages::certificate_and_key_request::message_type == task_type_name) {
              task = this->process(scope, client_messages::certificate_and_key_request(task_object));
            }
            else if (client_messages::register_server_request::message_type == task_type_name) {
              task = this->process(scope, client_messages::register_server_request(task_object));
            }
            //TODO: else if (consensus_messages::consensus_message_who_is_leader::message_type == task_type_name) {
            //  task = scope.get<iserver>().consensus_server_protocol().process(scope, consensus_messages::consensus_message_who_is_leader(task_object));
            //}
            else if (client_messages::put_file_message::message_type == task_type_name) {
              task = this->process(scope, client_messages::put_file_message(task_object));
            }
            else {
              this->log_.warning("Invalid request type \'%s\'", task_type_name.c_str());
              throw new std::runtime_error("Invalid request type " + task_type_name);
            }
            
            task.wait(
              [this, request_id](const json_value * task_result){
                auto obj = task_result->clone();
                dynamic_cast<json_object *>(obj.get())->add_property("$r", request_id);

                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.result = std::move(obj);
                this->task_mutex_.unlock();
              },
              [this, request_id](std::exception_ptr ex) {
                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.error = ex;
                this->task_mutex_.unlock();
              });
            
            this->task_mutex_.lock();
            p = this->tasks_.find(request_id);
            if(this->tasks_.end() != p){
              if(p->second.result){
                result->add(p->second.result->clone());
              }
            }
            this->task_mutex_.unlock();
          }
        }
      }
    }
  }

  return result.release();
}

vds::async_task<const vds::json_value *>
vds::_server_json_client_api::process(
  const service_provider & scope,
  const client_messages::certificate_and_key_request & message)
{
  return create_async_task(
    [scope, message](const std::function<void(const json_value *)> & done, const error_handler & on_error){
      auto cert = scope
        .get<istorage_log>()
        .find_cert(message.object_name());

      if (!cert
        || cert->password_hash() != message.password_hash()) {
        throw std::runtime_error("Invalid username or password");
      }

      std::unique_ptr<const_data_buffer> buffer = scope
        .get<istorage_log>()
        .get_object(cert->object_id());


      if (buffer) {
        binary_deserializer s(*buffer);
        object_container obj_cont(s);

        done(
          client_messages::certificate_and_key_response(
            obj_cont.get("c"),
            obj_cont.get("k")).serialize().get());
      }
    });
}

vds::async_task<const vds::json_value *>
vds::_server_json_client_api::process(
  const service_provider & scope,
  const client_messages::register_server_request & message)
{
  return create_async_task(
    [scope, message](const std::function<void(const json_value *)> & done, const error_handler & on_error){
      scope
        .get<node_manager>()
        .register_server(scope, message.certificate_body())
        .wait(
          [done]() { done(client_messages::register_server_response().serialize().get()); },
          on_error);
      });
}

vds::async_task<const vds::json_value *>
vds::_server_json_client_api::process(
  const service_provider & scope,
  const client_messages::put_file_message & message)
{
  auto version_id = guid::new_guid().str();
  
  return scope
    .get<istorage_log>()
    .save_file(
      version_id,
      message.user_login(),
      message.name(),
      message.tmp_file())
    .then([version_id](const std::function<void(const vds::json_value *)> & done, const error_handler & on_error) {
    done(client_messages::put_file_message_response(version_id).serialize().get());
  });
}
