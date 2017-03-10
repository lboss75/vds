/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "peer_network_schema.h"
#include "peer_network_schema_p.h"

vds::peer_network_schema::peer_network_schema(
  _peer_network_schema * impl)
  : impl_(impl)
{
  this->impl_->owner_ = this;
}

vds::peer_network_schema::~peer_network_schema()
{
  delete this->impl_;
}

const std::string & vds::peer_network_schema::schema() const
{
  return this->impl_->schema();
}

vds::event_source<vds::peer_channel *> & vds::peer_network_schema::open_channel(const std::string & address)
{
  return this->impl_->open_channel(address);
}
std::unique_ptr<vds::peer_network_schema> vds::peer_network_schema::udp_schema(const service_provider & sp)
{
  return std::unique_ptr<peer_network_schema>(new peer_network_schema(new _udp_network_schema(sp)));
}
//////////////////////////////////////////////////
vds::_peer_network_schema::_peer_network_schema(
  const service_provider & sp,
  const std::string & schema)
  : sp_(sp), schema_(schema)
{
}

vds::_peer_network_schema::~_peer_network_schema()
{
}

const std::string & vds::_peer_network_schema::schema() const
{
  return this->schema_;
}

vds::event_source<vds::peer_channel *> & vds::_peer_network_schema::open_channel(const std::string & address)
{
  std::lock_guard<std::mutex> lock(this->peer_channels_mutex_);

  auto p = this->peer_channels_.find(address);
  if (this->peer_channels_.end() != p) {
    return *p->second;
  }

  return *this->peer_channels_.set(address, this->_open_channel(address));
}

//////////////////////////////////////////////////
vds::_udp_network_schema::_udp_network_schema(const service_provider & sp)
  : _peer_network_schema(sp, "udp"),
  channel_(new peer_channel(new upd_peer_channel(sp, this)))
{
}

std::unique_ptr<vds::event_source<vds::peer_channel *>> vds::_udp_network_schema::_open_channel(const std::string & address)
{
  return std::unique_ptr<event_source<peer_channel *>>(new event_source<peer_channel *>([this](event_source<peer_channel *> & s) {
    s(this->channel_.get());
  }));
}
////////////////////////////////////////////////////
vds::_udp_network_schema::upd_peer_channel::upd_peer_channel(
  const service_provider & sp,
  _udp_network_schema * owner)
  : s_(sp), owner_(owner)
{
}

vds::_udp_network_schema::upd_peer_channel::~upd_peer_channel()
{
}


