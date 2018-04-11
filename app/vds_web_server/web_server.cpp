/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <queue>
#include <private/file_upload_task_p.h>
#include <private/api_controller.h>
#include "web_server.h"
#include "private/web_server_p.h"
#include "http_parser.h"
#include "tcp_network_socket.h"
#include "http_serializer.h"
#include "http_multipart_reader.h"
#include "file_operations.h"
#include "string_format.h"
#include "http_pipeline.h"
#include "user_manager.h"
#include "private/register_page.h"
#include "private/auth_session.h"
#include "private/login_page.h"
#include "private/index_page.h"

vds::web_server::web_server()
: port_(8050) {
}

vds::web_server::~web_server() {
}

void vds::web_server::register_services(service_registrator&) {
}

void vds::web_server::start(const service_provider& sp) {
  auto scope = sp.create_scope("Web server");
  mt_service::enable_async(scope);
  this->impl_ = std::make_shared<_web_server>(scope);
  this->impl_->start(scope, this->root_folder_, this->port_);
}

void vds::web_server::stop(const service_provider& sp) {
  this->impl_.reset();
}

vds::async_task<> vds::web_server::prepare_to_stop(const service_provider& sp) {
  return this->impl_->prepare_to_stop(sp);
}

vds::_web_server * vds::web_server::operator->() const {
  return this->impl_.get();
}
/////////////////////////////////////////////////////////////
vds::_web_server::_web_server(const service_provider& sp) 
: middleware_(*this) {

}

vds::_web_server::~_web_server() {
}

struct session_data : public std::enable_shared_from_this<session_data> {
  vds::tcp_network_socket s_;
  std::shared_ptr<vds::http_async_serializer> stream_;
  std::shared_ptr<vds::http_pipeline> handler_;

  session_data(vds::tcp_network_socket && s)
    : s_(std::move(s)),
      stream_(std::make_shared<vds::http_async_serializer>(this->s_)) {
  }
};

void vds::_web_server::start(const service_provider& sp, const std::string & root_folder, uint16_t port) {
  this->load_web("/", foldername(root_folder));
  this->server_.start(sp, network_address::any_ip4(port), [sp, pthis = this->shared_from_this()](tcp_network_socket s) {
    auto session = std::make_shared<session_data>(std::move(s));
    session->handler_ = std::make_shared<http_pipeline>(
        sp,
        session->stream_,
        [sp, pthis, session](const http_message & request) -> async_task<http_message> {
      if(request.headers().empty()) {
        session->stream_.reset();
        session->handler_.reset();
        return async_task<http_message>::empty();
      }

      std::string keep_alive_header;
      bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
      return pthis->middleware_.process(sp, request);
    });
    session->s_.start(sp, *session->handler_);
  }).execute([sp](const std::shared_ptr<std::exception> & ex) {
    if(ex) {
      sp.get<logger>()->trace(ThisModule, sp, "%s at web server", ex->what());
    }
  });
}

vds::async_task<> vds::_web_server::prepare_to_stop(const service_provider& sp) {
  return vds::async_task<>::empty();
}

vds::async_task<vds::http_message> vds::_web_server::route(
  const service_provider& sp,
  const http_message& message) {

  http_request request(message);
  if(request.url() == "/api/channels") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, message);
      if (!user_mng) {
        return vds::async_task<vds::http_message>::result(
            http_response::status_response(
                sp,
                http_response::HTTP_Unauthorized,
                "Unauthorized"));
      }

      const auto result = api_controller::get_channels(
          sp,
          *user_mng,
          this->shared_from_this(),
          message);
      return vds::async_task<vds::http_message>::result(
        http_response::simple_text_response(
          sp,
          result->str(),
          "application/json; charset=utf-8"));
    }

    if (request.method() == "POST") {
      auto user_mng = this->get_secured_context(sp, message);
      if (!user_mng) {
        return vds::async_task<vds::http_message>::result(
            http_response::status_response(
                sp,
                http_response::HTTP_Unauthorized,
                "Unauthorized"));
      }

      return index_page::create_channel(
          sp,
          user_mng,
          this->shared_from_this(),
          message);
    }
  }

  if (request.url() == "/api/login_state" && request.method() == "GET") {
    auto user_mng = this->get_secured_context(sp, message);
    if (!user_mng) {
      return vds::async_task<vds::http_message>::result(
        http_response::status_response(
          sp,
          http_response::HTTP_Unauthorized,
          "Unauthorized"));
    }

    return api_controller::get_login_state(
      sp,
      *user_mng,
      this->shared_from_this(),
      message);
  }

  if (request.url() == "/api/channel_feed") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, message);
      if (!user_mng) {
        return vds::async_task<vds::http_message>::result(
          http_response::status_response(
            sp,
            http_response::HTTP_Unauthorized,
            "Unauthorized"));
      }

      http_request request(message);
      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));

      return api_controller::channel_feed(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id)
        .then([sp](const std::shared_ptr<vds::json_value> & result) {

        return vds::async_task<vds::http_message>::result(
          http_response::simple_text_response(
            sp,
            result->str(),
            "application/json; charset=utf-8"));
      });
    }
  }

  if (request.url() == "/api/download") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, message);
      if (!user_mng) {
        return vds::async_task<vds::http_message>::result(
          http_response::status_response(
            sp,
            http_response::HTTP_Unauthorized,
            "Unauthorized"));
      }

      http_request request(message);
      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));
      const auto file_hash = base64::to_bytes(request.get_parameter("id"));

      return api_controller::download_file(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id,
        file_hash)
        .then([sp](
          const std::string & content_type,
          size_t body_size,
          const std::shared_ptr<vds::async_buffer<uint8_t>> & output_stream) {

        return vds::async_task<vds::http_message>::result(
          http_response::file_response(
            sp,
            output_stream,
            content_type,
            body_size));
      });
    }
  }

  if(request.url() == "/upload" && request.method() == "POST") {
    auto user_mng = this->get_secured_context(sp, message);
    if (!user_mng) {
      return vds::async_task<vds::http_message>::result(
        http_response::status_response(
          sp,
          http_response::HTTP_Unauthorized,
          "Unauthorized"));
    }

    return index_page::create_message(
      sp,
      user_mng,
      this->shared_from_this(),
      message);  
  }

  if (request.url() == "/" && request.method() == "GET") {
    const auto user_mng = this->get_secured_context(sp, message);
    if (nullptr == user_mng) {
      return vds::async_task<vds::http_message>::result(this->router_.route(sp, message, "/login"));
    }
    else {
      return vds::async_task<vds::http_message>::result(this->router_.route(sp, message, "/index"));
    }
  }

  if (request.url() == "/register" && request.method() == "POST") {
    return register_page::post(sp, this->shared_from_this(), message);
  }

  if (request.url() == "/login" && request.method() == "POST") {
    return login_page::post(sp, this->shared_from_this(), message);
  }

  return vds::async_task<vds::http_message>::result(this->router_.route(sp, message, request.url()));
}

void vds::_web_server::load_web(const std::string& path, const foldername & folder) {
  foldername f(folder);
  f.files([this, path](const filename & fn) -> bool{
    if(".html" == fn.extension()) {
      this->router_.add_file(path + fn.name_without_extension(), fn);
    }
    else {
      this->router_.add_file(path + fn.name(), fn);
    }
    return true;
  });
  f.folders([this, path](const foldername & fn) -> bool {
    this->load_web(path + fn.name() + "/", fn);
    return true;
  });
}

void vds::_web_server::add_auth_session(
    const std::string &id,
    const std::shared_ptr<vds::auth_session> &session) {

  std::unique_lock<std::shared_mutex> lock(this->auth_session_mutex_);
  this->auth_sessions_.emplace(id, session);

}

std::shared_ptr<vds::user_manager> vds::_web_server::get_secured_context(
    const service_provider & sp,
    const vds::http_message &message) const {

  std::string cookie_value;
  if(message.get_header("Cookie", cookie_value) && !cookie_value.empty()) {
    for (auto item : split_string(cookie_value, ';', true)) {
      auto p = item.find('=');
      if(std::string::npos != p) {
        auto name = item.substr(0, p);
        if("Auth" == name) {
          auto session_id = item.substr(p + 1);
          trim(session_id);

          std::shared_lock<std::shared_mutex> lock(this->auth_session_mutex_);
          auto p = this->auth_sessions_.find(session_id);
          if(this->auth_sessions_.end() != p){
            auto session = p->second;
            lock.unlock();

            return session->get_secured_context(sp);
          }
        }
      }
    }
  }

  return nullptr;
}

