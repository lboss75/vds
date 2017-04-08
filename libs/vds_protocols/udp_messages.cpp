/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "udp_messages.h"

vds::udp_messages::hello_message::hello_message(network_deserializer& s)
{
  s >> this->source_certificate_ >> this->session_id_ >> this->to_url_;
  s.final();
}

vds::udp_messages::hello_message::hello_message(
  const std::string & source_certificate,
  uint32_t session_id,
  const std::string& to_url)
: source_certificate_(source_certificate),
  session_id_(session_id),
  to_url_(to_url)
{
}

void vds::udp_messages::hello_message::serialize(network_serializer & s) const
{
  s.start(message_identification::hello_message_id);
  s << this->source_certificate_ << this->session_id_ << this->to_url_;
  s.final();
}

vds::udp_messages::welcome_message::welcome_message(
  const guid & server_id,
  const const_data_buffer & key_crypted,
  const const_data_buffer & crypted_info,
  const const_data_buffer & sign)
  : server_id_(server_id),
  key_crypted_(key_crypted),
  crypted_info_(crypted_info),
  sign_(sign)
{
}

void vds::udp_messages::welcome_message::serialize(network_serializer & s) const
{
  s << this->server_id_ << this->key_crypted_ << this->crypted_info_ << this->sign_;
}
