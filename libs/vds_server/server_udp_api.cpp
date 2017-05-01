/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_udp_api.h"
#include "server_udp_api_p.h"
#include "crypto_exception.h"
#include "cert_manager.h"

vds::server_udp_api::server_udp_api()
: impl_(new _server_udp_api(this))
{
}

vds::server_udp_api::~server_udp_api()
{
  delete this->impl_;
}

void vds::server_udp_api::start(const service_provider & sp)
{
  this->impl_->start(sp);
}

void vds::server_udp_api::stop(const service_provider & sp)
{
  this->impl_->stop(sp);
}

/////////////////////////////////////////////////////////////////
vds::_server_udp_api::_server_udp_api(
  server_udp_api * owner)
: owner_(owner)
{
}

vds::_server_udp_api::~_server_udp_api()
{
}

void vds::_server_udp_api::start(const service_provider & sp)
{
}

void vds::_server_udp_api::stop(const service_provider & sp)
{
}

