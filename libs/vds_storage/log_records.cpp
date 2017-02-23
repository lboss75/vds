/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"

std::unique_ptr<vds::json_object> vds::server_log_record::serialize()
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("f", this->fingerprint_);
  result->add_property("s", this->signature_);
  
  result->add_property(new json_property("m", this->message_.release()));
  
  return std::move(result);
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

std::unique_ptr<vds::json_object> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$type", message_type);
  
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  
  return result;
}

void vds::server_log_root_certificate::deserialize(const vds::json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if(nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("k", this->private_key_);
  }
}

const char vds::server_log_new_user_certificate::message_type[] = "certificate";

std::unique_ptr<vds::json_object> vds::server_log_new_user_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$type", message_type);
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  
  return result;
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

std::unique_ptr<vds::json_object> vds::server_log_batch::serialize()
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$type", message_type);

  if (0 != this->message_id_) {
    result->add_property("i", std::to_string(this->message_id_));
  }

  if (0 != this->previous_message_id_) {
    result->add_property("p", std::to_string(this->previous_message_id_));
  }

  result->add_property(new json_property("m", this->messages_.release()));

  return result;
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

const char vds::server_log_new_server::message_type[] = "new server";

std::unique_ptr<vds::json_object> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$type", message_type);
  result->add_property("c", this->certificate_);
  result->add_property("a", this->addresses_);
  return result;
}

void vds::server_log_new_server::deserialize(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("a", this->addresses_);
  }
}
