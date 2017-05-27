/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_messages.h"
#include "messages.h"
#include "network_serializer.h"

const char vds::consensus_messages::consensus_message_who_is_leader::message_type[] = "who is leader";

vds::consensus_messages::consensus_message_who_is_leader::consensus_message_who_is_leader(
  const const_data_buffer & source_id)
  : source_id_(source_id)
{
}

vds::consensus_messages::consensus_message_who_is_leader::consensus_message_who_is_leader(
  const_data_buffer && source_id)
  : source_id_(std::move(source_id))
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

void vds::consensus_messages::consensus_message_who_is_leader::serialize(vds::network_serializer& s) const
{
  s.start((uint8_t)vds::message_identification::who_is_leader_message_id);
  s << this->source_id_;
  s.final();  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
const char vds::consensus_messages::consensus_message_leader_candidate::message_type[] = "leader candidate";

vds::consensus_messages::consensus_message_leader_candidate::consensus_message_leader_candidate(
  const const_data_buffer & source_id)
: source_id_(source_id)
{
}

vds::consensus_messages::consensus_message_leader_candidate::consensus_message_leader_candidate(
  const_data_buffer && source_id)
: source_id_(std::move(source_id))
{
}

vds::consensus_messages::consensus_message_leader_candidate::consensus_message_leader_candidate(
  const vds::json_value* value)
{
 auto s = dynamic_cast<const json_object *> (value);
  if (nullptr != s) {
    s->get_property("s", this->source_id_);
  }
}

std::unique_ptr< vds::json_value > vds::consensus_messages::consensus_message_leader_candidate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("s", this->source_id_);

  return std::unique_ptr<json_value>(result.release());
}

void vds::consensus_messages::consensus_message_leader_candidate::serialize(vds::network_serializer& s) const
{
  s.start((uint8_t)vds::message_identification::leader_candidate_message_id);
  s << this->source_id_;
  s.final();  
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
const char vds::consensus_messages::consensus_message_current_leader::message_type[] = "i am leader";

vds::consensus_messages::consensus_message_current_leader::consensus_message_current_leader(
  const const_data_buffer & leader_id)
  : leader_id_(leader_id)
{
}

vds::consensus_messages::consensus_message_current_leader::consensus_message_current_leader(
  const_data_buffer && leader_id)
  : leader_id_(std::move(leader_id))
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
const char vds::consensus_messages::consensus_message_new_leader::message_type[] = "new leader";

vds::consensus_messages::consensus_message_new_leader::consensus_message_new_leader(
  const vds::const_data_buffer& leader_id)
  : leader_id_(leader_id)
{

}

vds::consensus_messages::consensus_message_new_leader::consensus_message_new_leader(const vds::json_value* value)
{
  auto s = dynamic_cast<const json_object *> (value);
  if (nullptr != s) {
    s->get_property("l", this->leader_id_);
  }
}

void vds::consensus_messages::consensus_message_new_leader::serialize(
  vds::network_serializer& s) const
{
  s.start((uint8_t)vds::message_identification::leader_candidate_message_id);
  s << this->leader_id_;
  s.final();  
}

std::unique_ptr< vds::json_value > vds::consensus_messages::consensus_message_new_leader::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());

  result->add_property("$t", message_type);
  result->add_property("l", this->leader_id_);

  return std::unique_ptr<json_value>(result.release());
}
