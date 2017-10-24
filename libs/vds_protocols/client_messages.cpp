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
  size_t order_num,
  const std::list<guid> & parents)
: id_(id),
  certificate_body_(certificate_body),
  private_key_body_(private_key_body),
  order_num_(order_num),
  parents_(parents)
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
    s->get_property("p", this->parents_);
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
  result->add_property("p", this->parents_);

  return std::shared_ptr<vds::json_value>(result.release());
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::server_log_state_request::message_type[] = "server log state";

vds::client_messages::server_log_state_request::server_log_state_request(const std::shared_ptr<json_value> & value)
{
}

std::shared_ptr<vds::json_value> vds::client_messages::server_log_state_request::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);

  return result;
}
///////////////////////////////////////////////////////////////////////////////
const char vds::client_messages::server_log_state_response::message_type[] = "server log state response";

vds::client_messages::server_log_state_response::server_log_state_response(
  size_t order_num,
  const std::list<guid> & parents)
: order_num_(order_num),
  parents_(parents)
{
}

vds::client_messages::server_log_state_response::server_log_state_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("o", this->order_num_);
    s->get_property("p", this->parents_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::server_log_state_response::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  result->add_property("o", this->order_num_);
  result->add_property("p", this->parents_);

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
    s->get_property("h", this->file_hash_);
    s->get_property("v", this->version_id_);

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
  s->add_property("m", this->principal_msg_.serialize(false));
  s->add_property("s", this->signature_);
  s->add_property("v", this->version_id_);
  s->add_property("f", this->tmp_file_.full_name());
  s->add_property("h", this->file_hash_);

  return s;
}

vds::client_messages::put_object_message::put_object_message(
  const guid & principal_id,
  const principal_log_record & principal_msg,
  const const_data_buffer & signature,
  const guid & version_id,
  const filename & tmp_file,
  const const_data_buffer & file_hash)
: principal_id_(principal_id),
  principal_msg_(principal_msg),
  signature_(signature),
  version_id_(version_id),
  tmp_file_(tmp_file),
  file_hash_(file_hash)
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
///////////////////////////////
const char vds::client_messages::principal_log_request::message_type[] = "get principal log";

vds::client_messages::principal_log_request::principal_log_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("p", this->principal_id_);
    s->get_property("o", this->last_order_num_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::principal_log_request::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  s->add_property("p", this->principal_id_);
  s->add_property("o", this->last_order_num_);
  return s;
}
///////////////////////////////
const char vds::client_messages::principal_log_response::message_type[] = "get principal log response";

vds::client_messages::principal_log_response::principal_log_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("p", this->principal_id_);
    s->get_property("o", this->last_order_num_);
    s->get_property("r", this->records_);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::principal_log_response::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  s->add_property("p", this->principal_id_);
  s->add_property("o", this->last_order_num_);
  s->add_property("r", this->records_);
  return s;
}

///////////////////////////////
const char vds::client_messages::get_object_request::message_type[] = "get object";

vds::client_messages::get_object_request::get_object_request(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    s->get_property("v", this->version_id_);
    
    std::string tmp_file;
    s->get_property("f", tmp_file);
    
    this->tmp_file_ = filename(tmp_file);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::get_object_request::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);
  s->add_property("v", this->version_id_);
  s->add_property("f", this->tmp_file_.full_name());
  return s;
}
///////////////////////////////
const char vds::client_messages::get_object_response::message_type[] = "get object response";

vds::client_messages::get_object_response::get_object_response(const std::shared_ptr<json_value> & value)
{
  auto s = std::dynamic_pointer_cast<json_object>(value);
  if (s) {
    std::string state;
    s->get_property("s", state);
    
    if("quered" == state){
      this->state_.status = task_state::task_status::QUERED;
    } else if("in progress" == state){
      this->state_.status = task_state::task_status::IN_PROGRESS;
    } else if("paused" == state){
      this->state_.status = task_state::task_status::PAUSED;
    } else if("failed" == state){
      this->state_.status = task_state::task_status::FAILED;
    } else if("done" == state){
      this->state_.status = task_state::task_status::DONE;
    }      
    
    s->get_property("t", this->state_.current_task);
    s->get_property("p", this->state_.progress_percent);
  }
}

std::shared_ptr<vds::json_value> vds::client_messages::get_object_response::serialize() const
{
  auto s = std::make_shared<json_object>();
  s->add_property("$t", message_type);

  switch(this->state_.status){
    case task_state::task_status::QUERED:
      s->add_property("s", "quered");
      break;
      
    case task_state::task_status::IN_PROGRESS:
      s->add_property("s", "in progress");
      break;
      
    case task_state::task_status::PAUSED:
      s->add_property("s", "paused");
      break;
      
    case task_state::task_status::FAILED:
      s->add_property("s", "failed");
      break;

    case task_state::task_status::DONE:
      s->add_property("s", "done");
      break;
  }
  s->add_property("t", this->state_.current_task);
  s->add_property("p", this->state_.progress_percent);

  return s;
}
///////////////////////////////
const char vds::client_messages::principal_log_add_record_request::message_type[] = "add record";
const char vds::client_messages::principal_log_add_record_response::message_type[] = "add record response";
