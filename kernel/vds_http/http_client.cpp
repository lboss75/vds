/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_client.h"
#include "logger.h"
#include "http_pipeline.h"
#include "private/tcp_network_socket_p.h"

vds::expected<void> vds::http_client::start(
	const service_provider * sp,
  const std::shared_ptr<tcp_network_socket> & s,
  const std::shared_ptr<vds::stream_output_async<uint8_t>> & output_stream) {

  auto eof = std::make_shared<async_result<vds::expected<void>>>();

  this->output_ = std::make_shared<http_async_serializer>(sp, output_stream);


  auto reader = std::make_shared<client_pipeline>(
	  sp,
    this->shared_from_this(),
    [pthis = this->shared_from_this(), eof](http_message message) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    decltype(pthis->response_handler_) f;
    pthis->response_handler_.swap(f);

    return f(std::move(message));
  });

  std::shared_ptr<_read_socket_task>  handler(new _read_socket_task(sp, s, reader));
  return handler->start();
}

vds::async_task<vds::expected<void>> vds::http_client::send(
  vds::http_message message,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler)
{
  vds_assert(!this->response_handler_);
  vds_assert(!this->final_handler_);

  this->response_handler_ = std::move(response_handler);

  async_result<expected<void>> result;
  this->final_handler_ = [&result](expected<void> r) -> async_task<expected<void>> {
    result.set_value(std::move(r));
    co_return expected<void>();
  };

  GET_EXPECTED_ASYNC(stream, co_await this->output_->start_message(message.headers()));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));
  co_return co_await result.get_future();
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::http_client::send(
  vds::http_message message,
  lambda_holder_t<async_task<expected<void>>, http_message> response_handler)
{
  vds_assert(!this->response_handler_);
  vds_assert(!this->final_handler_);

  async_result<expected<const_data_buffer>> result;
  this->response_handler_ = [&result, h = std::move(response_handler)](http_message message) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {
    
    CHECK_EXPECTED_ASYNC(co_await h(std::move(message)));

    co_return std::make_shared<collect_data>([&result](const_data_buffer response_body) -> async_task<expected<void>> {
      result.set_value(std::move(response_body));
      co_return expected<void>();
    });
  };

  async_result<expected<void>> completed;
  this->final_handler_ = [&completed](expected<void> r) -> async_task<expected<void>> {
    completed.set_value(std::move(r));
    co_return expected<void>();
  };

  GET_EXPECTED_ASYNC(stream, co_await this->output_->start_message(message.headers()));
  CHECK_EXPECTED_ASYNC(co_await stream->write_async(nullptr, 0));
  CHECK_EXPECTED_ASYNC(co_await completed.get_future());

  co_return co_await result.get_future();
}

vds::async_task<vds::expected<void>> vds::http_client::send_headers(
  vds::http_message message,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> response_handler,
  lambda_holder_t<async_task<expected<void>>, std::shared_ptr<stream_output_async<uint8_t>>> body_callback)
{
  vds_assert(!this->response_handler_);
  vds_assert(!this->final_handler_);

  this->response_handler_ = std::move(response_handler);

  async_result<expected<void>> result;
  this->final_handler_ = [&result](expected<void> r) -> async_task<expected<void>> {
    result.set_value(std::move(r));
    co_return expected<void>();
  };

  GET_EXPECTED_ASYNC(stream, co_await this->output_->start_message(message.headers()));
  CHECK_EXPECTED_ASYNC(co_await body_callback(stream));
  co_return co_await result.get_future();
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::http_client::send_headers(
  vds::http_message message,
  lambda_holder_t<async_task<expected<void>>, http_message> response_handler,
  lambda_holder_t<async_task<expected<void>>, std::shared_ptr<stream_output_async<uint8_t>>> body_callback)
{
  vds_assert(!this->response_handler_);
  vds_assert(!this->final_handler_);

  async_result<expected<const_data_buffer>> result;
  this->response_handler_ = [&result, h = std::move(response_handler)](http_message message)->async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    CHECK_EXPECTED_ASYNC(co_await h(std::move(message)));

    co_return std::make_shared<collect_data>([&result](const_data_buffer response_body) -> async_task<expected<void>> {
      result.set_value(std::move(response_body));
      return expected<void>();
    });
  };

  async_result<expected<void>> completed;
  this->final_handler_ = [&completed](expected<void> r) -> async_task<expected<void>> {
    completed.set_value(std::move(r));
    return expected<void>();
  };

  GET_EXPECTED_ASYNC(stream, co_await this->output_->start_message(message.headers()));
  CHECK_EXPECTED_ASYNC(co_await body_callback(stream));
  CHECK_EXPECTED_ASYNC(co_await completed.get_future());
  co_return co_await result.get_future();
}

vds::async_task<vds::expected<void>> vds::http_client::close() {
  return this->output_->stop();
}

vds::http_client::client_pipeline::client_pipeline(
  const service_provider * sp,
  std::shared_ptr<http_client> owner,
  lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, http_message> message_callback)
  : http_parser(sp, std::move(message_callback)), owner_(owner) {
}

vds::async_task<vds::expected<void>> vds::http_client::client_pipeline::finish_message()
{
  decltype(this->owner_->final_handler_) f;
  this->owner_->final_handler_.swap(f);

  return f(expected<void>());
}

vds::async_task<vds::expected<void>> vds::http_client::client_pipeline::before_close()
{
  if (!this->owner_->final_handler_) {
    return vds::expected<void>();
  }

  decltype(this->owner_->final_handler_) f;
  this->owner_->final_handler_.swap(f);

  return f(make_unexpected<std::runtime_error>("Network connection has been closed"));
}
