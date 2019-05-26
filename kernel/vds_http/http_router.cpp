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

vds::http_route_handler::http_route_handler(const std::string & url, const std::string & body)
  : url_(url), method_("GET"),
  handler_(
    [body](
      const service_provider * /*sp*/,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* /*router*/,
      const http_message& /*request*/) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
        CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(output_stream, body));
        co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
    }) {
}

vds::http_route_handler::http_route_handler(const std::string & url, const filename & fn)
  : url_(url), method_("GET"),
  handler_(
    [fn](
      const service_provider * /*sp*/,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* /*router*/,
      const http_message& /*request*/) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  auto content_type = http_mimetype::mimetype(fn);
  if (content_type.empty()) {
    content_type = "application/octet-stream";
  }

  GET_EXPECTED_ASYNC(body, file::read_all_text(fn));
  CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(output_stream, body, content_type));
  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();

}) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method,
  const std::function<
    async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>
    (
      const service_provider*,
      const std::shared_ptr<http_async_serializer>&,
      const std::shared_ptr<user_manager>&,
      const http_message&)>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* router,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  
  auto user_mng = router->auth_callback()(request);
  if (!user_mng) {
    CHECK_EXPECTED_ASYNC(co_await http_response::status_response(output_stream, http_response::HTTP_Unauthorized, "Unauthorized"));
    co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
  }

  co_return co_await callback(sp, output_stream, user_mng, request);
}) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method, 
  const std::function<
  async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>(
    const service_provider*,
    const std::shared_ptr<http_async_serializer>&output_stream,
    const http_message&)>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* /*router*/,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
    return callback(sp, output_stream, request);
  }) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method,
  const std::function<
    async_task<expected<void>>(
      const service_provider*,
      const std::shared_ptr<http_async_serializer>&,
      const http_message& )>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* router,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  CHECK_EXPECTED_ASYNC(co_await callback(
    sp,
    output_stream,
    request));

  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
}) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method,
  const std::function<
    async_task<expected<std::shared_ptr<json_value>>>(
      const service_provider*,
      const http_message&)>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* router,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  GET_EXPECTED_ASYNC(result, co_await callback(sp, request));
  GET_EXPECTED_ASYNC(result_str, result->str());
  CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(
    output_stream,
    result_str,
    "application/json; charset=utf-8"));

  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
}) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method,
  const std::function<
    async_task<expected<void>>(
      const service_provider*,
      const std::shared_ptr<http_async_serializer>&,
      const std::shared_ptr<user_manager>&,
      const http_message&)>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* router,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  auto user_mng = router->auth_callback()(request);
  if (!user_mng) {
    CHECK_EXPECTED_ASYNC(co_await http_response::status_response(
      output_stream,
      http_response::HTTP_Unauthorized,
      "Unauthorized"));
  }
  else {
    CHECK_EXPECTED_ASYNC(co_await callback(
      sp,
      output_stream,
      user_mng,
      request));
  }

  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
}) {
}

vds::http_route_handler::http_route_handler(
  const std::string & url,
  const std::string & method,
  const std::function<
    async_task<expected<std::shared_ptr<json_value>>>(
      const service_provider*,
      const std::shared_ptr<user_manager>&,
      const http_message&)>& callback)
  : url_(url), method_(method),
  handler_(
    [callback](
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router* router,
      const http_message& request) -> vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> {
  auto user_mng = router->auth_callback()(request);
  if (!user_mng) {
    CHECK_EXPECTED_ASYNC(co_await http_response::status_response(
      output_stream,
      http_response::HTTP_Unauthorized,
      "Unauthorized"));
  }
  else {
    GET_EXPECTED_ASYNC(result, co_await callback(sp, user_mng, request));
    GET_EXPECTED_ASYNC(result_str, result->str());
    CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(
      output_stream,
      result_str,
      "application/json; charset=utf-8"));
  }
  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();

}) {
}


vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::http_router::route(
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
  if (!std::get<0>(result)) {
    CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(
      output_stream,
      std::string(),
      std::string(),
      http_response::HTTP_Not_Found,
      "Not Found"));
    co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
  }
  else {
    co_return std::get<1>(result);
  }

  co_return make_unexpected<std::runtime_error>("Invalid programm");
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

