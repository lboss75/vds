/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_json_api.h"
#include "server_json_api_p.h"
#include "server.h"
#include "node_manager.h"


std::shared_ptr<vds::json_value> vds::server_json_api::operator()(
  const service_provider & scope,
  const std::shared_ptr<json_value> & request) const
{
  return static_cast<const _server_json_api *>(this)->operator()(scope, request);
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<vds::json_value> vds::_server_json_api::operator()(
  const service_provider & scope, 
  const std::shared_ptr<json_value> & request) const
{
  //auto cert = this->tunnel_.get_tunnel_certificate();
  //sp.get<logger>().trace("Certificate subject %s", cert.subject().c_str());
  //sp.get<logger>().trace("Certificate issuer %s", cert.issuer().c_str());

  auto result = std::make_shared<json_array>();

  auto request_tasks = std::dynamic_pointer_cast<json_array>(request);
  if (request_tasks) {
    for (size_t i = 0; i < request_tasks->size(); ++i) {
      auto task = request_tasks->get(i);

      auto task_object = std::dynamic_pointer_cast<json_object>(task);
      if (task_object) {
        auto task_type = task_object->get_property("$t");
        if (nullptr != task_type) {
          auto task_type_value = std::dynamic_pointer_cast<json_primitive>(task_type);
          if (task_type_value) {
            auto task_type_name = task_type_value->value();

            if(false) {
            }
            else {
              scope.get<logger>()->warning(scope, "Invalid request type \'%s\'", task_type_name.c_str());
            }
          }
        }
      }
    }
  }

  return result;
}
