/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "install_node_messages.h"

const char vds::install_node_prepare::message_type[] = "node prepare";

vds::json_object * vds::install_node_prepare::serialize()
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("u", this->user_id);
  result->add_property("p", this->password_hash);
  result->add_property("r", this->request_id);

  return result.release();
}

void vds::install_node_prepare::deserialize(const json_object * souce)
{
  souce->get_property("u", this->user_id);
  souce->get_property("p", this->password_hash);
  souce->get_property("r", this->request_id);
}

const char vds::install_node_prepared::message_type[] = "node prepared";
vds::json_object * vds::install_node_prepared::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("u", this->user_id);
  result->add_property("c", this->user_certificate);
  result->add_property("k", this->user_private_key);
  result->add_property("r", this->request_id);
  result->add_property("s", this->new_certificate_serial);

  return result.release();
}

void vds::install_node_prepared::deserialize(const json_object * source)
{
  source->get_property("u", this->user_id);
  source->get_property("c", this->user_certificate);
  source->get_property("k", this->user_private_key);
  source->get_property("r", this->request_id);
  source->get_property("s", this->new_certificate_serial);
}
