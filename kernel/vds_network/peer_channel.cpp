/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "peer_channel.h"
#include "peer_channel_p.h"

vds::peer_channel::peer_channel(_peer_channel * impl)
  : 
  formatter_type_(impl->get_formatter_type()),
  channel_direction_(impl->get_channel_direction()),
  impl_(impl)
{
  this->impl_->owner_ = this;
}

vds::peer_channel::~peer_channel()
{
  delete this->impl_;
}

void vds::peer_channel::broadcast(const std::string& data)
{
  if(formatter_type::json != this->formatter_type_){
    throw new std::runtime_error("Invalid message format");
  }
  
  this->impl_->broadcast(data);

}

void vds::peer_channel::broadcast(const const_data_buffer& data)
{
  if(formatter_type::binary != this->formatter_type_){
    throw new std::runtime_error("Invalid message format");
  }
  
  this->impl_->broadcast(data);
}

////////////////////////////////////////////////////////////////
vds::_peer_channel::_peer_channel()
{
}

vds::_peer_channel::~_peer_channel()
{
}
