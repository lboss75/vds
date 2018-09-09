/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_socket_server.h"
#include "private/tcp_socket_server_p.h"

vds::tcp_socket_server::tcp_socket_server()
: impl_(new _tcp_socket_server())
{
}

vds::tcp_socket_server::~tcp_socket_server()
{
}

std::future<void> vds::tcp_socket_server::start(
  const vds::service_provider& sp,
  const network_address & address,
  const std::function<void(tcp_network_socket s)>& new_connection)
{
  return this->impl_->start(sp, address, new_connection);
}

void vds::tcp_socket_server::stop(const vds::service_provider& sp)
{
  this->impl_->stop(sp);
}



