/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "peer_network.h"
#include "peer_network_p.h"

vds::peer_network::peer_network(const service_provider& sp)
: impl_(new _peer_network(sp, this))
{
}

vds::peer_network::~peer_network()
{
}

void vds::peer_network::for_each_active_channel(const std::function<void(peer_channel *)>& callback)
{
  this->impl_->for_each_active_channel(callback);
}

void vds::peer_network::register_handler(vds::peer_network::message_handler_base* handler)
{
  this->impl_->register_handler(handler);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
vds::_peer_network::_peer_network(
  const service_provider & sp,
  peer_network * owner)
: sp_(sp),
  owner_(owner)
{
}

void vds::_peer_network::for_each_active_channel(const std::function<void(peer_channel *)> & callback)
{
}

void vds::_peer_network::register_handler(vds::peer_network::message_handler_base * handler)
{
  if(this->handler_by_type_.end() != this->handler_by_type_.find(handler->json_message_type())
    || this->handler_by_type_id_.end() != this->handler_by_type_id_.find(handler->binary_message_type())){
    throw new std::invalid_argument("Handler alredy registered");
  }
  
  this->handlers_.push_back(std::unique_ptr<peer_network::message_handler_base>(handler));
  this->handler_by_type_[handler->json_message_type()] = handler;
  this->handler_by_type_id_[handler->binary_message_type()] = handler;
}

