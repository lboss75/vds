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

vds::const_data_buffer vds::udp_messages::hello_message::serialize() const
{
  network_serializer s;

  s.start(message_identification::hello_message_id);
  s << this->source_certificate_ << this->session_id_ << this->to_url_;
  s.final();

  return s.data();
}

vds::udp_messages::welcome_message::welcome_message(
  const std::string & server_certificate,
  const const_data_buffer & key_crypted,
  const const_data_buffer & crypted_data,
  const const_data_buffer & sign)
  : server_certificate_(server_certificate),
  key_crypted_(key_crypted),
  crypted_data_(crypted_data),
  sign_(sign)
{
}

vds::udp_messages::welcome_message::welcome_message(network_deserializer& s)
{
  s >> this->server_certificate_ >> this->key_crypted_ >> this->crypted_data_ >> this->sign_;
  s.final();
}

vds::const_data_buffer  vds::udp_messages::welcome_message::serialize() const
{
  network_serializer s;

  s.start(message_identification::welcome_message_id);
  s << this->server_certificate_ << this->key_crypted_ << this->crypted_data_ << this->sign_;
  s.final();

  return s.data();
}
