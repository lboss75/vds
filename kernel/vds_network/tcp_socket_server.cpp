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

vds::expected<void> vds::tcp_socket_server::start(
  const service_provider * sp,
  const network_address & address,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, std::shared_ptr<tcp_network_socket>> new_connection)
{
  return this->impl_->start(sp, address, std::move(new_connection));
}

void vds::tcp_socket_server::stop()
{
  this->impl_->stop();
}



