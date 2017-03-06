/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_messages.h"

const char vds::consensus_messages::consensus_message_who_is_leader::message_type[] = "who is leader";

vds::consensus_messages::consensus_message_who_is_leader::consensus_message_who_is_leader(const std::string & source_id)
  : source_id_(source_id)
{
}

vds::consensus_messages::consensus_message_who_is_leader::consensus_message_who_is_leader(const json_value * value)
{
  auto s = dynamic_cast<const json_object *> (value);
  if (nullptr != s) {
    s->get_property("s", this->source_id_);
  }
}

std::unique_ptr<vds::json_value> vds::consensus_messages::consensus_message_who_is_leader::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("s", this->source_id_);

  return std::unique_ptr<json_value>(result.release());
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
const char vds::consensus_messages::consensus_message_current_leader::message_type[] = "i am leader";

vds::consensus_messages::consensus_message_current_leader::consensus_message_current_leader(const std::string & leader_id)
  : leader_id_(leader_id)
{
}

vds::consensus_messages::consensus_message_current_leader::consensus_message_current_leader(const json_value * value)
{
  auto s = dynamic_cast<const json_object *> (value);
  if (nullptr != s) {
    s->get_property("l", this->leader_id_);
  }
}

std::unique_ptr<vds::json_value> vds::consensus_messages::consensus_message_current_leader::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("l", this->leader_id_);

  return std::unique_ptr<json_value>(result.release());
}
//////////////////////////////////////////////////////////////////////////////////////////////////