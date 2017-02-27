/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_api.h"
#include "server.h"
#include "node_manager.h"

vds::server_json_api::server_json_api(
  const service_provider & sp
)
: sp_(sp),
  log_(sp, "Server JSON API")
{
}

vds::json_value * vds::server_json_api::operator()(
  const service_provider & scope, 
  const json_value * request) const
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
        auto task_type = task_object->get_property("$t");
        if (nullptr != task_type) {
          auto task_type_value = dynamic_cast<const json_primitive *>(task_type);
          if (nullptr != task_type_value) {
            auto task_type_name = task_type_value->value();

            if (client_messages::certificate_and_key_request::message_type == task_type_name) {
              this->process(result.get(), client_messages::certificate_and_key_request(task_object));
            }
            else {
              this->log_.warning("Invalid request type %s", task_type_name.c_str());
            }
          }
        }
      }
    }
  }

  return result.release();
}

void vds::server_json_api::process(json_array * result, const client_messages::certificate_and_key_request & message) const
{
  std::string cert_body;
  std::string key_body;
  if (this->sp_.get<iserver>().get_storage_log()->get_cert_and_key(message.object_name(), message.password_hash(), cert_body, key_body)) {
    result->add(client_messages::certificate_and_key_response(message.request_id(), cert_body, key_body).serialize());
  }
  else {
    result->add(client_messages::certificate_and_key_response(message.request_id(), "Invalid username or password").serialize());
  }
}
