/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_router.h"
#include "http_response.h"
#include "http_outgoing_stream.h"
#include "http_request.h"
#include "http_mimetype.h"


vds::async_task<vds::expected<void>> vds::http_route_handler::static_handler::process(
  const service_provider * /*sp*/,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_router* /*router*/,
  const http_message& /*request*/) const {
  return http_response::simple_text_response(output_stream, this->body_);
}

vds::async_task<vds::expected<void>> vds::http_route_handler::file_handler::process(
  const service_provider * /*sp*/,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_router * router,
  const http_message & /*request*/) const {

  auto content_type = http_mimetype::mimetype(this->fn_);
  if(content_type.empty()) {
    content_type = "application/octet-stream";
  }

  GET_EXPECTED(body, file::read_all_text(this->fn_));
  return http_response::simple_text_response(output_stream, body, content_type);
}

vds::async_task<vds::expected<void>> vds::http_route_handler::auth_handler::process(
  const service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_router* router,
  const http_message & request) const {

  auto user_mng = router->auth_callback()(request);
  if (!user_mng) {
    return http_response::status_response(
      output_stream,
      http_response::HTTP_Unauthorized,
      "Unauthorized");
  }

  return this->callback_(
    sp,
    output_stream,
    user_mng,
    request);
}

vds::async_task<vds::expected<void>> vds::http_router::route(
  const service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & request) const
{
  for(auto & handler : this->handlers_) {
    if(handler.url() == request.url() && handler.method() == request.method()) {
      co_return co_await handler.process(sp, output_stream, this, request);
    }
  }

  GET_EXPECTED_ASYNC(result, co_await this->not_found_handler_(output_stream, request));
  if (!result) {
    co_return co_await http_response::simple_text_response(
      output_stream,
      std::string(),
      std::string(),
      http_response::HTTP_Not_Found,
      "Not Found");
  }
  else {
    co_return expected<void>();
  }
}

void vds::http_router::add_static(
  const std::string& url,
  const std::string& response)
{
  this->handlers_.push_back(http_route_handler(url, response));
}

void vds::http_router::add_file(
  const std::string & url,
  const filename & filename
)
{
  this->handlers_.push_back(http_route_handler(url, filename));
}

vds::http_route_handler::auth_api_handler::auth_api_handler(
  const std::function<async_task<expected<std::shared_ptr<json_value>>>(
    const service_provider*,
    const std::shared_ptr<user_manager> &,
    const http_message&)>& callback)
  : auth_handler([callback](
      const service_provider* sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const std::shared_ptr<user_manager> & user_mng,
      const http_message & request) ->async_task<expected<void>> {
    std::shared_ptr<json_value> result;
  GET_EXPECTED_VALUE_ASYNC(result, co_await callback(sp, user_mng, request));
  GET_EXPECTED_ASYNC(result_str, result->str());
  co_return co_await http_response::simple_text_response(
    output_stream,
    result_str,
    "application/json; charset=utf-8");
})
{
}

vds::async_task<vds::expected<void>> vds::http_route_handler::web_handler::process(
  const service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_router * /*router*/,
  const http_message & request) const
{
  return this->callback_(
    sp,
    output_stream,
    request);
}

vds::http_route_handler::api_handler::api_handler(
  const std::function<async_task<expected<std::shared_ptr<json_value>>>(
    const service_provider*,
    const http_message&)>& callback)
  : web_handler([callback](
      const service_provider* sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message& request) -> async_task<expected<void>> {
    std::shared_ptr<json_value> result;
    GET_EXPECTED_VALUE_ASYNC(result, co_await callback(sp, request));
    GET_EXPECTED_ASYNC(result_str, result->str());
    co_return co_await http_response::simple_text_response(
      output_stream,
      result_str,
      "application/json; charset=utf-8");
}) {
}
