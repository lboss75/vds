/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_api.h"
#include "server.h"

vds::server_json_api::server_json_api(
  const service_provider & sp,
  ssl_peer & peer
)
: sp_(sp),
  log_(sp, "Server JSON API"),
  peer_(peer)
{
}

vds::json_value * vds::server_json_api::operator()(const json_value * request) const
{
  auto cert = this->peer_.get_peer_certificate();
  this->log_.trace("Certificate subject %s", cert.subject().c_str());
  this->log_.trace("Certificate issuer %s", cert.issuer().c_str());

  auto request_tasks = dynamic_cast<const json_array *>(request);
  if (nullptr != request_tasks) {
    for (size_t i = 0; i < request_tasks->size(); ++i) {
      auto task = request_tasks->get(i);

      auto task_object = dynamic_cast<const json_object *>(task);
      if (nullptr != task_object) {
        auto task_type = task_object->get_property("$type");
        if (nullptr != task_type) {
          auto task_type_value = dynamic_cast<const json_primitive *>(task_type);
          if (nullptr != task_type_value) {
            auto task_type_name = task_type_value->value();

            if (vsr_new_client_message::message_type == task_type_name) {
              this->process(vsr_new_client_message::deserialize(task_object));
            }
          }
        }
      }
    }
  }

  auto result = new json_object();
  result->add_property(new json_property("successful", new json_primitive("true")));
  return result;
}

void vds::server_json_api::process(const vsr_new_client_message & message) const
{
  this->sp_.get<iserver>().vsr_server_protocol()->new_client();
}
