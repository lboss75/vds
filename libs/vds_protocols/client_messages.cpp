/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_messages.h"

const char vds::client_messages::certificate_and_key_request::message_type[] = "ask certificate and key";

vds::client_messages::certificate_and_key_request::certificate_and_key_request(
  const std::string & request_id,
  const std::string & object_name,
  const std::string & password_hash)
: request_id_(request_id),
  object_name_(object_name),
  password_hash_(password_hash)
{
}

vds::client_messages::certificate_and_key_request::certificate_and_key_request(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *>(value);
  if(nullptr != s){
    s->get_property("r", this->request_id_);
    s->get_property("n", this->object_name_);
    s->get_property("h", this->password_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::client_messages::certificate_and_key_request::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->request_id_);
  result->add_property("n", this->object_name_);
  result->add_property("h", this->password_hash_);

  return std::unique_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::certificate_and_key_response::message_type[] = "certificate and key";

vds::client_messages::certificate_and_key_response::certificate_and_key_response(
  const std::string & request_id,
  const std::string & certificate_body,
  const std::string & private_key_body)
  : request_id_(request_id),
  certificate_body_(certificate_body),
  private_key_body_(private_key_body)
{
}

vds::client_messages::certificate_and_key_response::certificate_and_key_response(
  const std::string & request_id,
  const std::string & error)
  : request_id_(request_id),
  error_(error)
{
}

vds::client_messages::certificate_and_key_response::certificate_and_key_response(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *>(value);
  if (nullptr != s) {
    s->get_property("r", this->request_id_);
    s->get_property("e", this->error_);
    s->get_property("c", this->certificate_body_);
    s->get_property("p", this->private_key_body_);
  }
}

std::unique_ptr<vds::json_value> vds::client_messages::certificate_and_key_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->request_id_);

  if (this->error_.empty()) {
    result->add_property("c", this->certificate_body_);
    result->add_property("p", this->private_key_body_);
  }
  else {
    result->add_property("e", this->error_);
  }

  return std::unique_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::register_server_request::message_type[] = "register server";

vds::client_messages::register_server_request::register_server_request(
  const std::string & request_id,
  const std::string & certificate_body)
  : request_id_(request_id),
  certificate_body_(certificate_body)
{
}

vds::client_messages::register_server_request::register_server_request(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *>(value);
  if (nullptr != s) {
    s->get_property("r", this->request_id_);
    s->get_property("c", this->certificate_body_);
  }
}

std::unique_ptr<vds::json_value> vds::client_messages::register_server_request::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->request_id_);
  result->add_property("c", this->certificate_body_);

  return std::unique_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::register_server_response::message_type[] = "certificate and key";

vds::client_messages::register_server_response::register_server_response(
  const std::string & request_id,
  const std::string & error)
  : request_id_(request_id),
  error_(error)
{
}

vds::client_messages::register_server_response::register_server_response(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *>(value);
  if (nullptr != s) {
    s->get_property("r", this->request_id_);
    s->get_property("e", this->error_);
  }
}

std::unique_ptr<vds::json_value> vds::client_messages::register_server_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("r", this->request_id_);

  if (!this->error_.empty()) {
    result->add_property("e", this->error_);
  }

  return std::unique_ptr<vds::json_value>(result.release());
}

