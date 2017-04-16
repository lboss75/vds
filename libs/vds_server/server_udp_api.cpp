/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_udp_api.h"
#include "server_udp_api_p.h"
#include "crypto_exception.h"
#include "cert_manager.h"

vds::server_udp_api::server_udp_api(
  const service_provider& sp)
: impl_(new _server_udp_api(sp, this))
{
}

vds::server_udp_api::~server_udp_api()
{
  delete this->impl_;
}

void vds::server_udp_api::start()
{
  this->impl_->start();
}

void vds::server_udp_api::stop()
{
  this->impl_->stop();
}

/////////////////////////////////////////////////////////////////
vds::_server_udp_api::_server_udp_api(
  const service_provider & sp,
  server_udp_api * owner)
: sp_(sp),
  log_(sp, "Server UDP API"),
  owner_(owner)
{
}

vds::_server_udp_api::~_server_udp_api()
{
}

