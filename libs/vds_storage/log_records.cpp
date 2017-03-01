/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"

std::unique_ptr<vds::json_value> vds::server_log_record::serialize()
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("f", this->fingerprint_);
  result->add_property("s", this->signature_);
  
  result->add_property(new json_property("m", this->message_.release()));
  
  return std::unique_ptr<vds::json_value>(result.release());
}

void vds::server_log_record::deserialize(vds::json_value* source)
{
  auto s = dynamic_cast<json_object *>(source);
  if(nullptr != s) {
    s->get_property("f", this->fingerprint_);
    s->get_property("s", this->signature_);
    this->message_ = s->move_property("m");
  }
}

const char vds::server_log_root_certificate::message_type[] = "root";

vds::server_log_root_certificate::server_log_root_certificate(
  const std::string & certificate,
  const std::string & private_key,
  const std::string & password_hash)
  : certificate_(certificate),
  private_key_(private_key),
  password_hash_(password_hash)
{
}

vds::server_log_root_certificate::server_log_root_certificate(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("k", this->private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  result->add_property("h", this->password_hash_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

const char vds::server_log_new_user_certificate::message_type[] = "certificate";

std::unique_ptr<vds::json_value> vds::server_log_new_user_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

void vds::server_log_new_user_certificate::deserialize(const vds::json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if(nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("k", this->private_key_);
  }
}

const char vds::server_log_batch::message_type[] = "batch";

std::unique_ptr<vds::json_value> vds::server_log_batch::serialize()
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);

  if (0 != this->message_id_) {
    result->add_property("i", std::to_string(this->message_id_));
  }

  if (0 != this->previous_message_id_) {
    result->add_property("p", std::to_string(this->previous_message_id_));
  }

  result->add_property(new json_property("m", this->messages_.release()));

  return std::unique_ptr<vds::json_value>(result.release());
}

void vds::server_log_batch::deserialize(json_value * source)
{
  auto s = dynamic_cast<json_object *>(source);
  if (nullptr != s) {
    s->get_property("i", this->message_id_);
    s->get_property("p", this->previous_message_id_);
    
    auto m = s->move_property("m");
    if(m){
      auto ma = dynamic_cast<json_array *>(m.get());
      this->messages_.reset(ma);
      m.release();
    }
    
  }
}
//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_server::message_type[] = "new server";

vds::server_log_new_server::server_log_new_server(
  const std::string& certificate)
: certificate_(certificate)
{
}


vds::server_log_new_server::server_log_new_server(
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->certificate_);
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_);
  return std::unique_ptr<vds::json_value>(result.release());
}
//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_endpoint::message_type[] = "new endpoint";

vds::server_log_new_endpoint::server_log_new_endpoint(
  const std::string & addresses)
  : addresses_(addresses)
{
}


vds::server_log_new_endpoint::server_log_new_endpoint(
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("a", this->addresses_);
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_endpoint::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("a", this->addresses_);
  return std::unique_ptr<vds::json_value>(result.release());
}


