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
  delete this->impl_;
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

void vds::_server_connection::get_delivery_metrics(std::map<std::string, size_t> & metrics)
{
}

void vds::_server_connection::send(const std::string & from_address, std::list<std::string> & to_address, const std::string &  body)
{
}

