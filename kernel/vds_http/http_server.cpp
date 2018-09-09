/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_server.h"
#include "private/http_server_p.h"
#include "logger.h"
#include "http_pipeline.h"

vds::http_server::http_server()
: impl_(new _http_server())
{
}

vds::http_server::~http_server()
{
}

std::future<void> vds::http_server::start(
  const vds::service_provider & sp,
  const std::shared_ptr<continuous_buffer<uint8_t>> & incoming_stream,
  const std::shared_ptr<continuous_buffer<uint8_t>> & outgoing_stream,
  const handler_type & handler)
{
  throw std::runtime_error("Not implemented");
/*
  return this->impl_->start(sp, this->impl_, incoming_stream, outgoing_stream, handler);
*/
}
