/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_client.h"
#include "logger.h"
#include "http_pipeline.h"


vds::async_task<vds::expected<void>> vds::http_client::start(
	const service_provider * sp,
  const std::shared_ptr<vds::stream_input_async<uint8_t>> & input_stream,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream) {

  auto eof = std::make_shared<async_result<vds::expected<void>>>();

  this->output_ = std::make_shared<http_async_serializer>(output_stream);
  this->pipeline_ = std::make_shared<client_pipeline>(
	sp,
    [pthis = this->shared_from_this(), eof](http_message && message) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    decltype(pthis->response_handler_) f;
    pthis->response_handler_.swap(f);

    return f(std::move(message));
  });

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::http_client::send(
  vds::http_message && message,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > && response_handler)
{
  GET_EXPECTED_ASYNC(stream, co_await this->send_headers(std::move(message), std::move(response_handler)));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));
  co_return expected<void>();
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http_client::send_headers(
  vds::http_message && message,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > && response_handler)
{
  vds_assert(!this->response_handler_);
  this->response_handler_ = std::move(response_handler);

  return this->output_->start_message(message.headers());
}


vds::async_task<vds::expected<void>> vds::http_client::close() {
  return this->output_->stop();
}

vds::http_client::client_pipeline::client_pipeline(
  const service_provider * sp,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message && > message_callback)
  : http_parser(sp, std::move(message_callback)) {
}
