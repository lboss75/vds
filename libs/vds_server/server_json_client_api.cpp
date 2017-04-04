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
              this->process(scope, result.get(), client_messages::certificate_and_key_request(task_object));
            }
            else if (client_messages::register_server_request::message_type == task_type_name) {
              this->process(scope, result.get(), client_messages::register_server_request(task_object));
            }
            else if (consensus_messages::consensus_message_who_is_leader::message_type == task_type_name) {
              scope.get<iserver>().consensus_server_protocol().process(scope, *result, consensus_messages::consensus_message_who_is_leader(task_object));
            }
            else if (client_messages::put_file_message::message_type == task_type_name) {
              this->process(scope, result.get(), client_messages::put_file_message(task_object));
            }
            else {
              this->log_.warning("Invalid request type \'%s\'", task_type_name.c_str());
              throw new std::runtime_error("Invalid request type " + task_type_name);
            }
          }
        }
      }
    }
  }

  return result.release();
}

void vds::_server_json_client_api::process(
  const service_provider & scope,
  json_array * result,
  const client_messages::certificate_and_key_request & message) const
{
  auto cert = scope
    .get<istorage>()
    .get_storage_log()
    .find_cert(message.object_name());

  if (!cert
    || cert->password_hash() != message.password_hash()) {
    result->add(client_messages::certificate_and_key_response(message.request_id(), "Invalid username or password").serialize());
  }

  std::unique_ptr<data_buffer> buffer = scope
    .get<istorage>()
    .get_storage_log()
    .get_object(cert->object_id());


  if (buffer) {
    binary_deserializer s(*buffer);
    object_container obj_cont(s);

    result->add(
      client_messages::certificate_and_key_response(
        message.request_id(),
        obj_cont.get("c"),
        obj_cont.get("k")).serialize());
  }
}

void vds::_server_json_client_api::process(
  const service_provider & scope,
  json_array * result,
  const client_messages::register_server_request & message) const
{
  std::string error;
  if (scope.get<iserver>().get_node_manager().register_server(scope, message.certificate_body(), error)) {
    result->add(client_messages::register_server_response(message.request_id(), error).serialize());
  }
}

void vds::_server_json_client_api::process(const service_provider & scope, json_array * result, const client_messages::put_file_message & message) const
{
  /*
  scope
    .get<istorage>()
    .get_storage_log()
    .save_file(
      message.user_login,
      base64::to_bytes(message.datagram));

  result->add(client_messages::put_file_message_response(message.request_id(), std::string()).serialize());
  */
}
