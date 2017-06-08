/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_messages.h"

const char vds::client_messages::certificate_and_key_request::message_type[] = "ask certificate and key";

vds::client_messages::certificate_and_key_request::certificate_and_key_request(
  const std::string & object_name,
  const const_data_buffer & password_hash)
: object_name_(object_name),
  password_hash_(password_hash)
{
}

vds::client_messages::certificate_and_key_request::certificate_and_key_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if(s){
    s->get_property("n", this->object_name_);
    s->get_property("h", this->password_hash_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::certificate_and_key_request::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("n", this->object_name_);
  result->add_property("h", this->password_hash_);

  return result;
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::certificate_and_key_response::message_type[] = "certificate and key";

vds::client_messages::certificate_and_key_response::certificate_and_key_response(
  const std::string & certificate_body,
  const std::string & private_key_body)
  :
  certificate_body_(certificate_body),
  private_key_body_(private_key_body)
{
}

vds::client_messages::certificate_and_key_response::certificate_and_key_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("c", this->certificate_body_);
    s->get_property("p", this->private_key_body_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::certificate_and_key_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  result->add_property("c", this->certificate_body_);
  result->add_property("p", this->private_key_body_);

  return std::shared_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::register_server_request::message_type[] = "register server";

vds::client_messages::register_server_request::register_server_request(
  const std::string & certificate_body)
  : certificate_body_(certificate_body)
{
}

vds::client_messages::register_server_request::register_server_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("c", this->certificate_body_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::register_server_request::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_body_);

  return result;
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::register_server_response::message_type[] = "certificate and key";

vds::client_messages::register_server_response::register_server_response()
{
}

vds::client_messages::register_server_response::register_server_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::register_server_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  return std::shared_ptr<vds::json_value>(result.release());
}
////////////////////////////////////////////////////////////////////////////////////
/*
const char vds::client_messages::put_object_message::message_type[] = "put file";
vds::client_messages::put_object_message::put_object_message(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("n", this->name_);
    s->get_property("u", this->user_login_);
    s->get_property("m", this->meta_info_);

    std::string v;
    if (s->get_property("f", v)) {
      this->tmp_file_ = filename(v);
    }
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::put_object_message::serialize() const
{
  std::unique_ptr<json_object> s(new json_object());
  s->add_property("$t", message_type);

  s->add_property("n", this->name_);
  s->add_property("m", this->meta_info_);
  s->add_property("u", this->user_login_);
  s->add_property("f", this->tmp_file_.full_name());

  return std::shared_ptr<vds::json_value>(s.release());
}

vds::client_messages::put_object_message::put_object_message(
  const std::string & user_login,
  const std::string & name,
  const const_data_buffer & meta_info,
  const filename & tmp_file)
  : 
  user_login_(user_login),
  name_(name),
  meta_info_(meta_info),
  tmp_file_(tmp_file)
{
}

const char vds::client_messages::put_object_message_response::message_type[] = "put file response";

vds::client_messages::put_object_message_response::put_object_message_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("i", this->version_id_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::put_object_message_response::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  s->add_property("i", this->version_id_);
  return s;
}

vds::client_messages::put_object_message_response::put_object_message_response(
  const std::string & version_id)
: version_id_(version_id)
{
}

const char vds::client_messages::get_file_message_request::message_type[] = "get file";
vds::client_messages::get_file_message_request::get_file_message_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("u", this->user_login_);
    s->get_property("n", this->name_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::get_file_message_request::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);

  s->add_property("u", this->user_login_);
  s->add_property("n", this->name_);

  return s;
}

vds::client_messages::get_file_message_request::get_file_message_request(
  const std::string & user_login,
  const std::string & name)
: user_login_(user_login),
  name_(name)

{
}

const char vds::client_messages::get_file_message_response::message_type[] = "get file response";
vds::client_messages::get_file_message_response::get_file_message_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    std::string v;
    if (s->get_property("f", v)) {
      this->tmp_file_ = filename(v);
    }
    s->get_property("m", this->meta_info_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::get_file_message_response::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  s->add_property("f", this->tmp_file_.full_name());
  s->add_property("m", this->meta_info_);

  return s;
}

vds::client_messages::get_file_message_response::get_file_message_response(
  const const_data_buffer & meta_info,
  const filename & tmp_file)
: meta_info_(meta_info), tmp_file_(tmp_file)
{
}
*/