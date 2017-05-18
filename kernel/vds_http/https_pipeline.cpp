/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
/*
#include "https_pipeline.h"
#include "https_pipeline_p.h"

vds::https_pipeline::https_pipeline(
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: impl_(new _https_pipeline(
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

void vds::https_pipeline::on_connected(const service_provider & sp)
{
}

void vds::https_pipeline::on_connection_closed(const service_provider & sp)
{
}

void vds::https_pipeline::on_error(const service_provider & sp, std::exception_ptr /*error* /)
{
}

void vds::https_pipeline::connect(const service_provider & sp)
{
  this->impl_->connect(sp);
}

const std::string & vds::https_pipeline::address() const
{
  return this->impl_->address();
}

int vds::https_pipeline::port() const
{
  return this->impl_->port();
}

void vds::https_pipeline::run(const service_provider & sp, const std::string & body)
{
  this->impl_->run(sp, body);
}

//////////////////////////////////////////////////////////////////////////
vds::_https_pipeline::_https_pipeline(
  https_pipeline * owner,
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: owner_(owner),
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

void vds::_https_pipeline::connect(const service_provider & parent_scope)
{
  auto sp = parent_scope.create_scope("_https_pipeline");
  dataflow(
    socket_connect(),
    connection(this)
  )
  (
   [this](const service_provider & sp){ this->owner_->on_connected(sp); },
   [this](const service_provider & sp, std::exception_ptr ex) { this->owner_->on_error(sp, ex); },
   sp,
   this->address_,
   this->port_
  );
}

vds::_https_pipeline::connection::connection(
  _https_pipeline * owner)
: owner_(owner)
{
}

void vds::_https_pipeline::run(const service_provider & sp, const std::string & body)
{
  this->output_command_stream_->run(sp, body);
}
*/