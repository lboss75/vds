/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_client.h"
#include "logger.h"
#include "http_pipeline.h"


vds::async_task<vds::expected<void>> vds::http_client::start(
  const std::shared_ptr<vds::stream_input_async<uint8_t>> & input_stream,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream) {

  auto eof = std::make_shared<async_result<vds::expected<void>>>();

  this->output_ = std::make_shared<http_async_serializer>(output_stream);
  this->pipeline_ = std::make_shared<client_pipeline>(
    [pthis = this->shared_from_this(), eof](const http_message message) -> async_task<expected<void>> {

    if (!message) {
      vds_assert(!pthis->result_);
      eof->set_value(expected<void>());
    }
    else {
      vds_assert(pthis->result_);
      decltype(pthis->response_handler_) f;
      pthis->response_handler_.swap(f);

      auto callback_result = co_await f(message);

      if (callback_result.has_error()) {
        auto result = std::move(pthis->result_);
        result->set_value(unexpected(std::move(callback_result.error())));
      }
      else {
        auto result = std::move(pthis->result_);
        result->set_value(expected<void>());
      }
    }
    co_return expected<void>();
  });

  CHECK_EXPECTED_ASYNC(co_await this->pipeline_->process(input_stream));
  CHECK_EXPECTED_ASYNC(co_await eof->get_future());

  co_return expected<void>();
}



vds::async_task<vds::expected<void>> vds::http_client::send(
  const vds::http_message message,
  const std::function<vds::async_task<vds::expected<void>>(vds::http_message response)> & response_handler)
{
  vds_assert(!this->result_);
  auto r = std::make_shared<async_result<vds::expected<void>>>();
  this->result_ = r;
  this->response_handler_ = response_handler;

  CHECK_EXPECTED_ASYNC(co_await this->output_->write_async(message));
  CHECK_EXPECTED_ASYNC(co_await r->get_future());

  co_return expected<void>();
}

vds::http_client::client_pipeline::client_pipeline(
  const std::function<vds::async_task<vds::expected<void>>(http_message message)>& message_callback)
  : http_parser(message_callback) {
}
