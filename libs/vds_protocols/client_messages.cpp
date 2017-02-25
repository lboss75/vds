/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_messages.h"

const char vds::client_messages::ask_certificate_and_key::message_type[] = "ask certificate and key";

vds::client_messages::ask_certificate_and_key::ask_certificate_and_key(
  const std::string & request_id,
  const std::string & object_name)
: request_id_(request_id),
  object_name_(object_name)
{
}

vds::client_messages::ask_certificate_and_key::ask_certificate_and_key(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *>(value);
  if(nullptr != s){
    s->get_property("r", this->request_id_);
    s->get_property("n", this->object_name_);
  }
}

std::unique_ptr<vds::json_object> vds::client_messages::ask_certificate_and_key::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("t", message_type);
  result->add_property("r", this->request_id_);
  result->add_property("n", this->object_name_);
  
  return result;
}

