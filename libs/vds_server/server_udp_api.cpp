/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_udp_api.h"
#include "server_udp_api_p.h"

vds::server_udp_api::server_udp_api(const service_provider& sp)
: impl_(new _server_udp_api(sp, this))
{
}

vds::server_udp_api::~server_udp_api()
{
  delete this->impl_;
}

void vds::server_udp_api::start(const std::string& address, size_t port)
{
  this->impl_->start(address, port);
}

void vds::server_udp_api::stop()
{
  this->impl_->stop();
}
/////////////////////////////////////////////////////////////////
vds::_server_udp_api::_server_udp_api(const service_provider & sp, server_udp_api * owner)
: sp_(sp),
owner_(owner),
s_(sp)
{
}

vds::_server_udp_api::~_server_udp_api()
{
}

void vds::_server_udp_api::start(const std::string& address, size_t port)
{
  run_udp_server<_server_udp_api>(this->s_, address, port, this);
}
