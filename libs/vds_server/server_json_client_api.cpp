/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_client_api.h"
#include "private/server_json_client_api_p.h"
#include "server.h"
#include "node_manager.h"
#include "storage_log.h"
#include "principal_record.h"
#include "server_database.h"
#include "parallel_tasks.h"
#include "private/principal_manager_p.h"
#include "private/server_database_p.h"
#include "private/server_log_sync_p.h"

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
  sp.get<logger>()->trace("JSON API", sp, "%s", request->str().c_str());

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

              async_task<std::shared_ptr<json_value>> task = async_task<std::shared_ptr<json_value>>::empty();

              if (client_messages::certificate_and_key_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::certificate_and_key_request(task_object));
              }
              else if (client_messages::register_server_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::register_server_request(task_object));
              }
              else if (client_messages::get_object_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::get_object_request(task_object));
              }
              else if (client_messages::put_object_message::message_type == task_type_name) {
                task = this->process(sp, client_messages::put_object_message(task_object));
              }
              else if (client_messages::principal_log_request::message_type == task_type_name) {
                task = this->process(sp, client_messages::principal_log_request(task_object));
              }
              else {
                sp.get<logger>()->warning("JSON API", sp, "Invalid request type \'%s\'", task_type_name.c_str());
                throw std::runtime_error("Invalid request type " + task_type_name);
              }

              task.execute(
                [sp, this, request_id](const std::shared_ptr<std::exception> & ex, const std::shared_ptr<json_value> & task_result) {
                  if(!ex){
                std::dynamic_pointer_cast<json_object>(task_result)->add_property("$r", request_id);

                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.result = task_result;
                this->task_mutex_.unlock();
              } else {
                auto error_id = guid::new_guid().str();
                sp.get<logger>()->error("JSON API", sp, "Error %s: %s", error_id.c_str(), ex->what());

                auto error_response = std::make_shared<json_object>();
                error_response->add_property("$r", request_id);
                error_response->add_property("$e", "Error " + error_id);

                this->task_mutex_.lock();
                auto p = this->tasks_.find(request_id);
                p->second.error = error_response;
                this->task_mutex_.unlock();
              }});
            }
          }
          catch (const std::exception & ex) {
            auto error_id = guid::new_guid().str();
            sp.get<logger>()->error("JSON API", sp, "Error %s: %s", error_id.c_str(), ex.what());

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
            sp.get<logger>()->error("JSON API", sp, "Error %s: Unexpected error", error_id.c_str());

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
  return [sp, message](const async_result<std::shared_ptr<json_value>> & result){
      (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
      [sp, result, message](database_transaction & tr){
        auto cert = sp
          .get<principal_manager>()->find_user_principal(sp, tr, message.object_name());

        if (!cert
          || cert->password_hash() != message.password_hash()) {
          result.error(std::make_shared<std::runtime_error>("Invalid username or password"));
        }
        else {
          std::list<guid> active_records;
          auto order_num = sp.get<principal_manager>()->get_current_state(sp, tr, active_records);

          result.done(
            client_messages::certificate_and_key_response(
              cert->id(),
              cert->cert_body().str(),
              base64::from_bytes(cert->cert_key()),
              order_num,
              active_records).serialize());
        }
        
        return true;
      });
    };
}

vds::async_task<std::shared_ptr<vds::json_value>>
vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::register_server_request & message)
{
  return [sp, message](const async_result<std::shared_ptr<json_value>> & result){
      
      (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
      [sp, result, message](database_transaction & tr) {
        barrier b;

        sp.get<node_manager>()->register_server(
          sp,
          tr,
          message.id(),
          message.parent_id(),
          certificate::parse(message.server_certificate()),
          base64::to_bytes(message.server_private_key()),
          message.password_hash())
          .execute(
            [result, &b, sp](const std::shared_ptr<std::exception> & ex) {
              if(!ex){
              b.set();
              result.done(client_messages::register_server_response().serialize());            
            } else {
              result.error(ex);
            }
            });
        b.wait();
        return true;
      });
    };
}


vds::async_task<std::shared_ptr<vds::json_value>>
vds::_server_json_client_api::process(
  const service_provider & sp,
  const client_messages::put_object_message & message)
{
  return [sp, message](
    const async_result<std::shared_ptr<vds::json_value>> & result){
      (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
        [sp, message, result](database_transaction & tr)->bool{
      
      principal_log_record record(message.principal_msg());
      
      std::cout << "Upload file " << record.id().str() << ", principal=" << record.principal_id().str() << "\n";
      
      auto author = sp.get<principal_manager>()->find_principal(sp, tr, record.principal_id());
      if(!author){
        result.error(std::make_shared<std::runtime_error>("Author not found"));
        return false;
      }
      
      auto body = message.principal_msg().serialize(false)->str();
      if(!asymmetric_sign_verify::verify(
          hash::sha256(),
          author->cert_body().public_key(),
          message.signature(),
          body.c_str(),
          body.length())){
        result.error(std::make_shared<std::runtime_error>("Signature verification failed"));
        return false;
      }
      
      principal_log_new_object new_object(record.message());
  
      barrier b;
      sp.get<ichunk_manager>()->add_object(
        sp,
        tr,
        message.version_id(),
        message.tmp_file(),
        message.file_hash())
      .execute([result, sp, &tr, &b, &record, &message](const std::shared_ptr<std::exception> & ex){
        if(!ex){
        
        for(auto & p : record.parents()) {
          auto state = (*sp.get<principal_manager>())->principal_log_get_state(sp, tr, p);
          if(_principal_manager::principal_log_state::tail != state){
            throw std::runtime_error("Invalid parent_id " + p.str());
          }
        }

        sp.get<principal_manager>()->save_record(sp, tr, record, message.signature());
        sp.get<_server_log_sync>()->on_new_local_record(sp, record, message.signature());

        b.set();
        result.done(client_messages::put_object_message_response().serialize());
        }
        else {
          result.error(ex);
        }
        });
      b.wait();
      return true;
    });
  };
}

vds::async_task<std::shared_ptr<vds::json_value>> vds::_server_json_client_api::process(
  const service_provider & scope,
  const client_messages::get_object_request & message)
{
  return scope.get<ilocal_cache>()->download_object(
    scope,
    message.version_id(),
    message.tmp_file())
  .then(
    [](const client_messages::task_state & state) {
    
      return client_messages::get_object_response(state).serialize();
    });
}

vds::async_task<std::shared_ptr<vds::json_value>>
  vds::_server_json_client_api::process(const service_provider & sp, const client_messages::principal_log_request & message)
{
  return [sp, message](const async_result<std::shared_ptr<vds::json_value>> & result) {

    (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
      [sp, message, result](database_transaction & tr)->bool{
        
      size_t last_order_num;
      std::list<principal_log_record> records;
      sp.get<principal_manager>()->get_principal_log(
        sp,
        tr,
        message.principal_id(),
        message.last_order_num(),
        last_order_num,
        records);
      
      result.done(client_messages::principal_log_response(message.principal_id(), last_order_num, records).serialize());
      return true;
    });
  };
}
