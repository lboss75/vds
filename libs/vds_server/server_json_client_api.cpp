/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_client_api.h"
#include "server_json_client_api_p.h"
#include "server.h"
#include "node_manager.h"
#include "storage_log.h"
#include "principal_record.h"
#include "server_database.h"

vds::server_json_client_api::server_json_client_api()
: impl_(new _server_json_client_api(this))
{
}

vds::server_json_client_api::~server_json_client_api()
{
  delete this->impl_;
}

std::shared_ptr<vds::json_value> vds::server_json_client_api::operator()(
  const service_provider & scope,
  const std::shared_ptr<json_value> & request) const
{
  return this->impl_->operator()(scope, request);
}

///////////////////////////////////////////////////////////////////////////////
vds::_server_json_client_api::_server_json_client_api(
  server_json_client_api * owner
)
: owner_(owner)
{
}

std::shared_ptr<vds::json_value> vds::_server_json_client_api::operator()(
  const service_provider & sp, 
  const std::shared_ptr<json_value> & request)
{
  //auto cert = this->tunnel_.get_tunnel_certificate();
  //sp.get<logger>().trace("Certificate subject %s", cert.subject().c_str());
  //sp.get<logger>().trace("Certificate issuer %s", cert.issuer().c_str());
  sp.get<logger>()->trace(sp, "JSON API %s", request->str().c_str());

  auto result = std::make_shared<json_array>();

  auto request_tasks = std::dynamic_pointer_cast<json_array>(request);
  if (request_tasks) {
    for (size_t i = 0; i < request_tasks->size(); ++i) {
      auto task = request_tasks->get(i);

      auto task_object = std::dynamic_pointer_cast<json_object>(task);
      if (nullptr != task_object) {
        std::string request_id;
        if(task_object->get_property("$r", request_id)){
          try {
            bool need_start;
            this->task_mutex_.lock();
            auto p = this->tasks_.find(request_id);
            if (this->tasks_.end() == p) {
              need_start = true;
              this->tasks_.set(request_id, task_info());
            }
            else {
              need_start = false;
              if (p->second.result) {
                result->add(p->second.result);
              }
            }
            this->task_mutex_.unlock();

            if (need_start) {
              std::string task_type_name;
              task_object->get_property("$t", task_type_name);

              async_task<std::shared_ptr<json_value>> task;

              if (client_messages::certificate_and_key_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::certificate_and_key_request(task_object));
              }
              else if (client_messages::register_server_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::register_server_request(task_object));
              }
              //TODO: else if (consensus_messages::consensus_message_who_is_leader::message_type == task_type_name) {
              //  task = scope.get<iserver>().consensus_server_protocol().process(scope, consensus_messages::consensus_message_who_is_leader(task_object));
              //}
              //else if (client_messages::put_object_message::message_type == task_type_name) {
              //  task = this->process(sp, client_messages::put_object_message(task_object));
              //}
              //else if (client_messages::get_file_message_request::message_type == task_type_name) {
              //  task = this->process(sp, client_messages::get_file_message_request(task_object));
              //}
              else {
                sp.get<logger>()->warning(sp, "Invalid request type \'%s\'", task_type_name.c_str());
                throw std::runtime_error("Invalid request type " + task_type_name);
              }

              task.wait(
                [this, request_id](const service_provider & sp, const std::shared_ptr<json_value> & task_result) {
                std::dynamic_pointer_cast<json_object>(task_result)->add_property("$r", request_id);

                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.result = task_result;
                this->task_mutex_.unlock();
              },
                [this, request_id](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                auto error_id = guid::new_guid().str();
                sp.get<logger>()->error(sp, "Error %s: %s", error_id.c_str(), ex->what());

                auto error_response = std::make_shared<json_object>();
                error_response->add_property("$r", request_id);
                error_response->add_property("$e", "Error " + error_id);

                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.error = error_response;
                this->task_mutex_.unlock();
              },
                sp);
            }
          }
          catch (const std::exception & ex) {
            auto error_id = guid::new_guid().str();
            sp.get<logger>()->error(sp, "Error %s: %s", error_id.c_str(), ex.what());

            auto error_response = std::make_shared<json_object>();
            error_response->add_property("$r", request_id);
            error_response->add_property("$e", "Error " + error_id);

            this->task_mutex_.lock();
            auto p = this->tasks_.find(request_id);
            p->second.error = error_response;
            this->task_mutex_.unlock();
          }
          catch (...) {
            auto error_id = guid::new_guid().str();
            sp.get<logger>()->error(sp, "Error %s: Unexpected error", error_id.c_str());

            auto error_response = std::make_shared<json_object>();
            error_response->add_property("$r", request_id);
            error_response->add_property("$e", "Unexpected error");

            this->task_mutex_.lock();
            auto p = this->tasks_.find(request_id);
            p->second.error = error_response;
            this->task_mutex_.unlock();
          }

          this->task_mutex_.lock();
          auto p = this->tasks_.find(request_id);
          if (this->tasks_.end() != p) {
            if (p->second.result) {
              result->add(p->second.result);
            }
            else if (p->second.error) {
              result->add(p->second.error);
            }
          }
          this->task_mutex_.unlock();
        }
      }
    }
  }

  return result;
}

vds::async_task<std::shared_ptr<vds::json_value>>
vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::certificate_and_key_request & message)
{
  return create_async_task(
    [message](const std::function<void(const service_provider & sp, std::shared_ptr<json_value>)> & done,
              const error_handler & on_error,
              const service_provider & sp){
      auto cert = sp
        .get<iserver_database>()->find_user_principal(sp, message.object_name());

      if (!cert
        || cert->password_hash() != message.password_hash()) {
        on_error(sp, std::make_shared<std::runtime_error>("Invalid username or password"));
      }
      else {
        std::list<guid> active_records;
        auto order_num = sp.get<iserver_database>()->get_current_state(sp, active_records);

        done(
          sp,
          client_messages::certificate_and_key_response(
            cert->id(),
            cert->cert_body(),
            cert->cert_key(),
            active_records,
            order_num).serialize());
      }
    });
}

vds::async_task<std::shared_ptr<vds::json_value>>
vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::register_server_request & message)
{
  return create_async_task(
    [sp, message](const std::function<void(const service_provider & sp, std::shared_ptr<json_value>)> & done, const error_handler & on_error, const service_provider & sp){
      sp.get<node_manager>()->register_server(
        sp,
        message.id(),
        message.parent_id(),
        message.server_certificate(),
        message.server_private_key(),
        message.password_hash())
        .wait(
          [done](const service_provider & sp) { done(sp, client_messages::register_server_response().serialize()); },
          on_error,
          sp);
      });
}

/*
vds::async_task<std::shared_ptr<vds::json_value>>
vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::put_object_message & message)
{
  auto version_id = guid::new_guid().str();
  
  return sp.get<file_manager>()->put_file(
      sp,
      version_id,
      message.user_login(),
      message.name(),
      message.meta_info(),
      message.tmp_file())
    .then(
      [version_id](const std::function<void(const service_provider & sp, std::shared_ptr<vds::json_value>)> & done,
                   const error_handler & on_error,
                   const service_provider & sp) {

    done(sp, client_messages::put_object_message_response(version_id).serialize());
  });
}

vds::async_task<std::shared_ptr<vds::json_value>> vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::get_file_message_request & message)
{
  return sp.get<file_manager>()->download_file(
      sp,
      message.user_login(),
      message.name())
    .then([](
      const std::function<void(const service_provider & sp, std::shared_ptr<json_value>)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const_data_buffer meta_info,
      filename tmp_file) {
    done(sp, client_messages::get_file_message_response(meta_info, tmp_file).serialize());
  });
}
*/