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
}

void vds::peer_network::register_handler(vds::peer_network::message_handler_base* handler)
{
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
}
