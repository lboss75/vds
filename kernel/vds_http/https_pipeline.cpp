/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "https_pipeline.h"
#include "https_pipeline_p.h"

vds::https_pipeline::https_pipeline(
  const vds::service_provider & sp,
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: impl_(new _https_pipeline(
  sp,
  this,
  address,
  port,
  client_certificate,
  client_private_key))
{
}

vds::https_pipeline::~https_pipeline()
{
}

void vds::https_pipeline::on_connected()
{
}

void vds::https_pipeline::on_connection_closed()
{
}

void vds::https_pipeline::on_error(std::exception_ptr /*error*/)
{
}

void vds::https_pipeline::connect()
{
  this->impl_->connect();
}

const std::string & vds::https_pipeline::address() const
{
  return this->impl_->address();
}

int vds::https_pipeline::port() const
{
  return this->impl_->port();
}

void vds::https_pipeline::run(const std::string & body)
{
  this->impl_->run(body);
}

//////////////////////////////////////////////////////////////////////////
vds::_https_pipeline::_https_pipeline(
  const service_provider & sp,
  https_pipeline * owner,
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: sp_(sp),
owner_(owner),
log_(sp, "HTTPS pipeline"),
address_(address),
port_(port),
client_certificate_(client_certificate),
client_private_key_(client_private_key),
output_command_stream_(nullptr)
{
}

vds::_https_pipeline::~_https_pipeline()
{
}

void vds::_https_pipeline::connect()
{
  auto sp = this->sp_.create_scope();
  dataflow(
    socket_connect(sp),
    connection(sp, this)
  )
  (
   [this](){ this->owner_->on_connected(); },
   [this](std::exception_ptr ex) { this->owner_->on_error(ex); },
   this->address_,
   this->port_
  );
}

vds::_https_pipeline::connection::connection(
  const service_provider & sp,
  _https_pipeline * owner)
: sp_(sp),
owner_(owner)
{
}

void vds::_https_pipeline::run(const std::string & body)
{
  this->output_command_stream_->run(body);
}
