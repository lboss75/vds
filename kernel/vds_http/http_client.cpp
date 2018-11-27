/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_client.h"
#include "logger.h"
#include "http_pipeline.h"


vds::async_task<void> vds::http_client::start(
  const std::shared_ptr<vds::stream_input_async<uint8_t>> & input_stream,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream) {

  auto eof = std::make_shared<async_result<void>>();

  this->output_ = std::make_shared<http_async_serializer>(output_stream);
  this->pipeline_ = std::make_shared<client_pipeline>(
    [pthis = this->shared_from_this(), eof](const http_message message) -> async_task<void> {

    if (!message) {
      vds_assert(!pthis->result_);
      eof->set_value();
    }
    else {
      vds_assert(pthis->result_);

      auto result = std::move(pthis->result_);
      result->set_value(message);
    }
    co_return;
  });

  co_await this->pipeline_->process(input_stream);
  co_await eof->get_future();
}



vds::async_task<vds::http_message> vds::http_client::send(const vds::http_message message)
{
  vds_assert(!this->result_);
  this->result_ = std::make_shared<async_result<vds::http_message>>();

  co_await this->output_->write_async(message);

  co_return co_await this->result_->get_future();
}

vds::http_client::client_pipeline::client_pipeline(
  const std::function<vds::async_task<void>(const http_message message)>& message_callback)
  : http_parser<client_pipeline>(message_callback) {
}
