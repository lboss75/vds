/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_socket_server.h"
#include "tcp_socket_server_p.h"

vds::tcp_socket_server::tcp_socket_server()
: impl_(new _tcp_socket_server())
{
}

vds::tcp_socket_server::~tcp_socket_server()
{
}

vds::async_task<> vds::tcp_socket_server::start(
  const vds::service_provider& sp,
  const std::string& address,
  int port,
  const std::function<void(const service_provider & sp, const tcp_network_socket & s)>& new_connection)
{
  return this->impl_->start(sp, address, port, new_connection);
}

void vds::tcp_socket_server::stop(const vds::service_provider& sp)
{
  this->impl_->stop(sp);
}



