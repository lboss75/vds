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
    : url_(std::move(origin.url_)),
      method_(std::move(origin.method_)),
      handler_(origin.handler_->clone()) {

    }

    http_route_handler(
      const std::string & url,
      const std::string & body)
    : url_(url), method_("GET"), handler_(new static_handler(body)) {
    }

    http_route_handler(
      const std::string & url,
      const filename & fn)
      : url_(url), method_("GET"), handler_(new file_handler(fn)) {
    }

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider * ,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback)
      : url_(url), method_(method), handler_(new auth_api_handler(callback)) {
    }

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<void>>(
        const service_provider * ,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback)
      : url_(url), method_(method), handler_(new auth_handler(callback)) {
    }

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider *,
        const http_message &)> & callback)
      : url_(url), method_(method), handler_(new api_handler(callback)) {
    }

    http_route_handler(
      const std::string & url,
      const std::string & method,
      const std::function<async_task<expected<void>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_message &)> & callback)
      : url_(url), method_(method), handler_(new web_handler(callback)) {
    }

    const std::string & url() const {
      return this->url_;
    }

    const std::string & method() const {
      return this->method_;
    }
    
    async_task<expected<void>> process(
      const service_provider * sp,
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_router * router,
      const http_message & request) const {
      return this->handler_->process(sp, output_stream, router, request);
    }

  private:
    class handler_base {
    public:
      virtual ~handler_base() {}

      virtual handler_base * clone() = 0;
      virtual async_task<expected<void>> process(
        const service_provider * sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_router * router,
        const http_message & request) const = 0;
    };

    class static_handler : public handler_base {
    public:
      static_handler(const std::string & body)
      : body_(body) {        
      }

      handler_base * clone() override {
        return new static_handler(this->body_);
      }

      async_task<expected<void>> process(
        const service_provider * sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_router * router,
        const http_message & request) const override;

    private:
      std::string body_;
    };

    class file_handler : public handler_base {
    public:
      file_handler(const filename & fn)
      : fn_(fn){        
      }

      handler_base * clone() override {
        return new file_handler(this->fn_);
      }

      async_task<expected<void>> process(
        const service_provider * sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_router * router,
        const http_message & request) const override;

    private:
      filename fn_;
    };

    class auth_handler : public handler_base {
    public:
      auth_handler(const std::function<async_task<expected<void>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback)
        : callback_(callback) {
      }

      handler_base * clone() override {
        return new auth_handler(*this);
      }

      async_task<expected<void>> process(
        const service_provider * sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_router * router,
        const http_message & request) const override;

    private:
      std::function<async_task<expected<void>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::shared_ptr<user_manager> &,
        const http_message &)> callback_;
    };

    class auth_api_handler : public auth_handler {
    public:
      auth_api_handler(const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider *,
        const std::shared_ptr<user_manager> &,
        const http_message &)> & callback);

      handler_base * clone() override {
        return new auth_api_handler(*this);
      }
    };

    class web_handler : public handler_base {
    public:
      web_handler(
        const std::function<async_task<expected<void>>(
          const service_provider *,
          const std::shared_ptr<http_async_serializer> & output_stream,
          const http_message &)> & callback)
        : callback_(callback) {
      }

      handler_base * clone() override {
        return new web_handler(*this);
      }

      async_task<expected<void>> process(
        const service_provider * sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const http_router * router,
        const http_message & request) const override;

    private:
      std::function<async_task<expected<void>>(
        const service_provider *,
        const std::shared_ptr<http_async_serializer> &,
        const http_message &)> callback_;
    };

    class api_handler : public web_handler {
    public:
      api_handler(const std::function<async_task<expected<std::shared_ptr<json_value>>>(
        const service_provider *,
        const http_message &)> & callback);

      handler_base * clone() override {
        return new api_handler(*this);
      }
    };

    std::string url_;
    std::string method_;
    std::unique_ptr<handler_base> handler_;
  };

  class http_router
  {
  public:
    http_router(std::initializer_list<http_route_handler> && handlers)
    : handlers_(std::move(handlers)){
    }

    vds::async_task<vds::expected<void>> route(
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

    void not_found_handler(const std::function<async_task<expected<bool>>(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message &)> & callback) {
      this->not_found_handler_ = callback;
    }

  private:
    std::list<http_route_handler> handlers_;

    std::function<std::shared_ptr<user_manager>(const http_message &)> auth_callback_;
    std::function<async_task<expected<bool>>(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const http_message &)> not_found_handler_;
  };
}

#endif // __VDS_HTTP_HTTP_ROUTER_H_
