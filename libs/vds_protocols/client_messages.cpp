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
  const guid & id,
  const std::string & certificate_body,
  const std::string & private_key_body,
  const std::list<guid> & active_records,
  size_t order_num)
: id_(id),
  certificate_body_(certificate_body),
  private_key_body_(private_key_body),
  active_records_(active_records),
  order_num_(order_num)
{
}

vds::client_messages::certificate_and_key_response::certificate_and_key_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("i", this->id_);
    s->get_property("c", this->certificate_body_);
    s->get_property("k", this->private_key_body_);
    s->get_property("o", this->order_num_);

    auto p = std::dynamic_pointer_cast<json_array>(s->get_property("p"));
    if (p) {
      for (size_t i = 0; i < p->size(); ++i) {
        auto item = std::dynamic_pointer_cast<json_primitive>(p->get(i));
        if (item) {
          this->active_records_.push_back(guid::parse(item->value()));
        }
      }
    }
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::certificate_and_key_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  result->add_property("i", this->id_);
  result->add_property("c", this->certificate_body_);
  result->add_property("k", this->private_key_body_);
  result->add_property("o", this->order_num_);

  auto p = std::make_shared<json_array>();
  for (auto & record : this->active_records_) {
    auto item = std::make_shared<json_primitive>(record.str());
    p->add(item);
  }
  result->add_property("p", p);


  return std::shared_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::register_server_request::message_type[] = "register server";

vds::client_messages::register_server_request::register_server_request(
  const guid & id,
  const guid & parent_id,
  const std::string & server_certificate,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
: id_(id),
  parent_id_(parent_id),
  server_certificate_(server_certificate),
  server_private_key_(server_private_key),
  password_hash_(password_hash)
{
}

vds::client_messages::register_server_request::register_server_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("i", this->id_);
    s->get_property("p", this->parent_id_);
    s->get_property("c", this->server_certificate_);
    s->get_property("k", this->server_private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::register_server_request::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("i", this->id_);
  result->add_property("p", this->parent_id_);
  result->add_property("c", this->server_certificate_);
  result->add_property("k", this->server_private_key_);
  result->add_property("h", this->password_hash_);

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
const char vds::client_messages::put_object_message::message_type[] = "put file";
vds::client_messages::put_object_message::put_object_message(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("i", this->principal_id_);
    this->principal_msg_ = s->get_property("m");
    s->get_property("s", this->signature_);

    std::string v;
    if (s->get_property("f", v)) {
      this->tmp_file_ = filename(v);
    }
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::put_object_message::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);

  s->add_property("i", this->principal_id_);
  s->add_property("m", this->principal_msg_);
  s->add_property("s", this->signature_);
  s->add_property("f", this->tmp_file_.full_name());

  return s;
}

vds::client_messages::put_object_message::put_object_message(
  const guid & principal_id,
  const std::shared_ptr<json_value> & principal_msg,
  const const_data_buffer & signature,
  const filename & tmp_file)
: principal_id_(principal_id),
  principal_msg_(principal_msg),
  signature_(signature),
  tmp_file_(tmp_file)
{
}

const char vds::client_messages::put_object_message_response::message_type[] = "put file response";

vds::client_messages::put_object_message_response::put_object_message_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::put_object_message_response::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  return s;
}

vds::client_messages::put_object_message_response::put_object_message_response()
{
}

/*
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