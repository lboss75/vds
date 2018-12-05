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


vds::async_task<vds::http_message> vds::http_route_handler::static_handler::process(
  const service_provider * sp,
  const http_router* router,
  const http_request& request) const {
  co_await request.get_message().ignore_empty_body();
  co_return http_response::simple_text_response(this->body_);
}

vds::async_task<vds::http_message> vds::http_route_handler::file_handler::process(
  const service_provider * sp,
  const http_router* router,
  const http_request & request) const {

  auto content_type = http_mimetype::mimetype(this->fn_);
  if(content_type.empty()) {
    content_type = "application/octet-stream";
  }

  co_await request.get_message().ignore_empty_body();
  co_return http_response::simple_text_response(file::read_all_text(this->fn_), content_type);
}

vds::async_task<vds::http_message> vds::http_route_handler::auth_handler::process(
  const service_provider * sp,
  const http_router* router,
  const http_request& request) const {

  auto user_mng = router->auth_callback()(request);
  if (!user_mng) {
    co_return http_response::status_response(
      http_response::HTTP_Unauthorized,
      "Unauthorized");
  }

  co_return co_await this->callback_(
    sp,
    user_mng,
    request);
}

vds::async_task<vds::http_message> vds::http_router::route(
  const service_provider * sp,
  const http_request & request) const
{
  for(auto & handler : this->handlers_) {
    if(handler.url() == request.url() && handler.method() == request.method()) {
      co_return co_await handler.process(sp, this, request);
    }
  }

  co_await request.get_message().ignore_empty_body();
  co_return http_response::simple_text_response(
    std::string(),
    std::string(),
    http_response::HTTP_Not_Found,
    "Not Found");
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
  const std::function<async_task<std::shared_ptr<json_value>>(const service_provider*, const std::shared_ptr<user_manager> &, const http_request&)>& callback)
  : auth_handler([callback](const service_provider* sp, const std::shared_ptr<user_manager> & user_mng, const http_request& request) ->async_task<http_message> {
  auto result = co_await callback(sp, user_mng, request);
  co_return http_response::simple_text_response(
    result->str(),
    "application/json; charset=utf-8");
})
{
}

vds::async_task<vds::http_message> vds::http_route_handler::web_handler::process(
  const service_provider * sp,
  const http_router * router,
  const http_request & request) const
{
  return this->callback_(
    sp,
    request);
}

vds::http_route_handler::api_handler::api_handler(
  const std::function<async_task<std::shared_ptr<json_value>>(const service_provider*, const http_request&)>& callback)
  : web_handler([callback](const service_provider* sp, const http_request& request) ->async_task<http_message> {
  auto result = co_await callback(sp, request);
  co_return http_response::simple_text_response(
    result->str(),
    "application/json; charset=utf-8");
})
{
}
