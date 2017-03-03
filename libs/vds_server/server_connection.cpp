/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_connection.h"
#include "server_connection_p.h"
#include "server.h"

vds::server_connection::server_connection(const service_provider & sp)
  : impl_(new _server_connection(sp, this))
{

}

vds::server_connection::~server_connection()
{
}

vds::consensus_protocol::iserver_gateway & vds::server_connection::get_server_gateway() const
{
  return *this->impl_.get();
}

///////////////////////////////////////////////////////////////////////////////////
vds::_server_connection::_server_connection(
  const service_provider & sp,
  server_connection * owner)
  : sp_(sp), owner_(owner)
{
}

vds::_server_connection::~_server_connection()
{
}

void vds::_server_connection::send(const std::list<std::string>& target_ids, const std::string & message)
{
}

void vds::_server_connection::broadcast(const std::string & message)
{
  this->sp_.get<iserver>().get_client_logic()->add_task(message);
}

