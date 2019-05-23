#ifndef __VDS_HTTP_HTTP_ROUTER_H_
#define __VDS_HTTP_HTTP_ROUTER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <map>
#include "http_serializer.h"

namespace vds {
  class http_message;
  class filename;
  class user_manager;
  class http_router;

  class http_route_handler {
  public:
    http_route_handler(http_route_handler && origin)
    : url_(std::move(origin.url_)),
      method_(std::move(origin.method_)),
      handler_(std::move(origin.handler_)) {      
    }

    http_route_handler(const http_route_handler & origin)
    : url_(origin.url_),
      method_(origin.method_),
      handler_(handler_) {

    }

    http_route_handler(
      const std::string & url,
      const std::string & body);

    http_route_handler(
      const std::string & url,
      const filename & fn);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider * ,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<void>>(
        const service_provider * ,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider *,
        const http_message &)> & callback);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<void>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_message &)> & callback);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_message &)> & callback);

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback);

    const std::string & url() const {
      return this->url_;
    }

    const std::string & method() const {
      return this->method_;
    }
    
    async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> process(
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router * router,
      const http_message & request) const {
      return this->handler_(sp, output_stream, router, request);
    }

  private:
    std::string url_;
    std::string method_;
    std::function<
      async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>
      (
      const service_provider * /*sp*/,
      const std::shared_ptr<http_async_serializer> & /*output_stream*/,
      const http_router * /*router*/,
      const http_message & /*request*/)> handler_;
  };

  class http_router
  {
  public:
    http_router(std::initializer_list<http_route_handler> && handlers)
    : handlers_(std::move(handlers)){
    }

    vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> route(
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message & request) const;
    
    void add_static(
      const std::string & url,
      const std::string & response);

    void add_file(
      const std::string & url,
      const filename & filename);

    const std::function<std::shared_ptr<user_manager>(const http_message &)> & auth_callback() const {
      return this->auth_callback_;
    }

    void auth_callback(const std::function<std::shared_ptr<user_manager> (const http_message &)> & callback) {
      this->auth_callback_ = callback;
    }

    void not_found_handler(const std::function<async_task<expected<std::tuple<bool, std::shared_ptr<stream_output_async<uint8_t>>>>>(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message &)> & callback) {
      this->not_found_handler_ = callback;
    }

  private:
    std::list<http_route_handler> handlers_;

    std::function<std::shared_ptr<user_manager>(const http_message &)> auth_callback_;
    std::function<async_task<expected<std::tuple<bool, std::shared_ptr<stream_output_async<uint8_t>>>>>(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message &)> not_found_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_ROUTER_H_
