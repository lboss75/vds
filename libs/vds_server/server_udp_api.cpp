/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_udp_api.h"
#include "server_udp_api_p.h"

vds::server_udp_api::server_udp_api(
  const service_provider& sp,
  certificate & certificate,
  asymmetric_private_key & private_key)
: impl_(new _server_udp_api(sp, this, certificate, private_key))
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
vds::_server_udp_api::_server_udp_api(
  const service_provider & sp,
  server_udp_api * owner,
  certificate & certificate,
  asymmetric_private_key & private_key)
: sp_(sp),
  owner_(owner),
  certificate_(certificate),
  private_key_(private_key),
  s_(sp)
{
}

vds::_server_udp_api::~_server_udp_api()
{
  //this->sp_.get<iserver>().get_connection_manager().remove_target(this);
}

void vds::_server_udp_api::start(const std::string& address, size_t port)
{
  run_udp_server(this->sp_, this->s_, address, port, *this);
}

void vds::_server_udp_api::stop()
{
}

void vds::_server_udp_api::udp_server_done()
{
}

void vds::_server_udp_api::udp_server_error(std::exception * ex)
{
}

void vds::_server_udp_api::socket_closed(std::list<std::exception *> errors)
{
}

void vds::_server_udp_api::input_message(const sockaddr_in & from, const void * data, size_t len)
{
}

void vds::_server_udp_api::update_upd_connection_pool()
{

}

