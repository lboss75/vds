/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_connection.h"
#include "server_connection_p.h"
#include "server.h"
#include "node_manager.h"
#include "server_udp_api.h"

vds::server_connection::server_connection(
  const service_provider & sp,
  server_udp_api * udp_api)
  : impl_(new _server_connection(sp, udp_api, this))
{

}

vds::server_connection::~server_connection()
{
  delete this->impl_;
}

void vds::server_connection::start()
{
  this->impl_->start();
}

void vds::server_connection::stop()
{
  this->impl_->stop();
}


///////////////////////////////////////////////////////////////////////////////////
vds::_server_connection::_server_connection(
  const service_provider & sp,
  server_udp_api * udp_api,
  server_connection * owner)
: sp_(sp), 
  log_(sp, "Server Connection"),
  udp_api_(udp_api),
  owner_(owner)
{
}

vds::_server_connection::~_server_connection()
{
}

void vds::_server_connection::start()
{
}

void vds::_server_connection::stop()
{
}

void vds::_server_connection::init_connection(const std::string & address, uint16_t port)
{

}


void vds::_server_connection::open_udp_session(const std::string & address)
{
}

void vds::_server_connection::open_https_session(const std::string & address)
{
  auto network = url_parser::parse_network_address(address);
  assert("https" == network.protocol);
  
}

