/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <queue>
#include "private/api_controller.h"
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
#include "private/auth_session.h"
#include "private/login_page.h"
#include "private/index_page.h"
#include "db_model.h"
#include "chunk_dbo.h"

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

std::future<void> vds::web_server::prepare_to_stop(const service_provider& sp) {
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
  std::shared_ptr<vds::stream_output_async<uint8_t>> target_;
  std::shared_ptr<vds::http_async_serializer> stream_;
  std::shared_ptr<vds::http_pipeline> handler_;

  session_data(const std::shared_ptr<vds::stream_output_async<uint8_t>> & target)
    : target_(target),
      stream_(std::make_shared<vds::http_async_serializer>(target)) {
  }
};

std::future<void> vds::_web_server::start(
    const service_provider& sp,
    const std::string & root_folder,
    uint16_t port) {
  this->load_web("/", foldername(root_folder));
  co_await this->server_.start(sp, network_address::any_ip4(port),
    [sp, pthis = this->shared_from_this()](std::shared_ptr<tcp_network_socket> s) -> std::future<void>{
    auto[reader, writer] = s->start(sp);
    auto session = std::make_shared<session_data>(writer);
    session->handler_ = std::make_shared<http_pipeline>(
        sp,
        session->stream_,
      [sp, pthis, session](const http_message request) -> std::future<http_message> {
      try {
        if (request.headers().empty()) {
          session->stream_.reset();
          session->handler_.reset();
          co_return http_message();
        }

        std::string keep_alive_header;
        //bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
        co_return co_await pthis->middleware_.process(sp, request);
      }
      catch (const std::exception & ex) {
        co_return http_response::status_response(http_response::HTTP_Internal_Server_Error, ex.what());
      }
    });
    co_await session->handler_->process(sp, reader);
  });
}

std::future<void> vds::_web_server::prepare_to_stop(const service_provider& sp) {
  co_return;
}

std::future<vds::http_message> vds::_web_server::route(
  const service_provider& sp,
  const http_message message) {
   
  http_request request(message);
  if(request.url() == "/api/channels") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
                http_response::HTTP_Unauthorized,
                "Unauthorized");
      }

      const auto result = api_controller::get_channels(
          sp,
          *user_mng,
          this->shared_from_this(),
          message);
      co_return http_response::simple_text_response(
          result->str(),
          "application/json; charset=utf-8");
    }

    if (request.method() == "POST") {
      auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
                http_response::HTTP_Unauthorized,
                "Unauthorized");
      }

      co_return co_await index_page::create_channel(
          sp,
          user_mng,
          this->shared_from_this(),
          message);
    }
  }

  if (request.url() == "/api/try_login" && request.method() == "GET") {
    const auto login = request.get_parameter("login");
    const auto password = request.get_parameter("password");

    co_return co_await api_controller::get_login_state(
      sp,
      login,
      password,
      this->shared_from_this(),
      message);
  }

  if (request.url() == "/api/session" && request.method() == "GET") {
    const auto session_id = request.get_parameter("session");

    co_return co_await api_controller::get_session(
      sp,
      this->shared_from_this(),
      session_id);
  }

  if (request.url() == "/api/logout" && request.method() == "POST") {
    const auto session_id = request.get_parameter("session");

    co_return co_await api_controller::logout(
      sp,
      this->shared_from_this(),
      session_id);
  }

  if (request.url() == "/api/channel_feed") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
            http_response::HTTP_Unauthorized,
            "Unauthorized");
      }

      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));

      auto result = co_await api_controller::channel_feed(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id);

      co_return http_response::simple_text_response(
            result->str(),
            "application/json; charset=utf-8");
    }
  }

  if (request.url() == "/api/devices") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
            http_response::HTTP_Unauthorized,
            "Unauthorized");
      }

      auto result = co_await
      api_controller::user_devices(
          sp,
          user_mng,
          this->shared_from_this());

      co_return http_response::simple_text_response(
          result->str(),
          "application/json; charset=utf-8");
    }
    if (request.method() == "POST") {
      auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
                http_response::HTTP_Unauthorized,
                "Unauthorized");
      }

      const auto device_name = request.get_parameter("name");
      const auto reserved_size = safe_cast<uint64_t>(std::atoll(request.get_parameter("size").c_str()));
      const auto local_path = request.get_parameter("path");
      co_await api_controller::lock_device(sp, user_mng, this->shared_from_this(), device_name, local_path,
                                         reserved_size);

      co_return http_response::status_response(
                    http_response::HTTP_OK,
                    "OK");
    }
  }

  if (request.url() == "/api/offer_device") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
                http_response::HTTP_Unauthorized,
                "Unauthorized");
      }

      auto result = co_await api_controller::offer_device(
          sp,
          user_mng,
          this->shared_from_this());

            co_return http_response::simple_text_response(
                    result->str(),
                    "application/json; charset=utf-8");
    }
  }

  if (request.url() == "/api/download") {
    if (request.method() == "GET") {
      co_await message.ignore_empty_body(sp);
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        co_return http_response::status_response(
            http_response::HTTP_Unauthorized,
            "Unauthorized");
      }

      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));
      const auto file_hash = base64::to_bytes(request.get_parameter("object_id"));

      auto buffer = std::make_shared<continuous_buffer<uint8_t>>();

      auto result = co_await api_controller::download_file(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id,
        file_hash,
        std::make_shared<continuous_stream_output_async<uint8_t>>(buffer));

        co_return http_response::file_response(
            std::make_shared<continuous_stream_input_async<uint8_t>>(buffer),
            result.size,
            result.mime_type,
            result.name);
    }
  }

  if (request.url() == "/api/parse_join_request" && request.method() == "POST") {
    const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      co_return http_response::status_response(
          http_response::HTTP_Unauthorized,
          "Unauthorized");
    }

    co_return co_await index_page::parse_join_request(
      sp,
      user_mng,
      this->shared_from_this(),
      message);
  }
  if (request.url() == "/approve_join_request" && request.method() == "POST") {
    auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      co_return http_response::status_response(
          http_response::HTTP_Unauthorized,
          "Unauthorized");
    }

    co_return co_await index_page::approve_join_request(
      sp,
      user_mng,
      this->shared_from_this(),
      message);
  }

  if(request.url() == "/upload" && request.method() == "POST") {
    auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      co_await message.ignore_body(sp);
      co_return http_response::status_response(
          http_response::HTTP_Unauthorized,
          "Unauthorized");
    }

    co_return co_await index_page::create_message(
      sp,
      user_mng,
      this->shared_from_this(),
      message);  
  }

  if (request.url() == "/" && request.method() == "GET") {
    co_return co_await this->router_.route(sp, message, "/index");
  }

  if (request.url() == "/api/register_requests" && request.method() == "GET") {
    co_await message.ignore_empty_body(sp);
    auto result = co_await api_controller::get_register_requests(sp, this->shared_from_this());
    co_return http_response::simple_text_response(
          result->str(),
          "application/json; charset=utf-8");
    
  }
  if (request.url() == "/api/register_request" && request.method() == "GET") {
    co_await message.ignore_empty_body(sp);
    auto result = co_await api_controller::get_register_request(
      sp,
      this->shared_from_this(),
      base64::to_bytes(request.get_parameter("id")));

    co_return http_response::simple_text_response(
      result->str(),
      "application/json; charset=utf-8");
  }

  if (request.url() == "/api/download_register_request" && request.method() == "GET") {
    const auto request_id = base64::to_bytes(request.get_parameter("id"));
    co_await message.ignore_empty_body(sp);

    auto result = co_await api_controller::get_register_request_body(sp, this->shared_from_this(), request_id);
    co_return http_response::file_response(
        result,
        "user_invite.bin");
  }

  if(request.url() == "/register_request" && request.method() == "POST") {
    co_return co_await login_page::register_request_post(sp, this->shared_from_this(), message);
  }

  if (request.url() == "/api/statistics") {
    if (request.method() == "GET") {
      co_await message.ignore_empty_body(sp);

      auto result = co_await api_controller::get_statistics(
        sp,
        this->shared_from_this(),
        message);
        co_return http_response::simple_text_response(
          result->str(),
          "application/json; charset=utf-8");
    }
  }

  co_return co_await this->router_.route(sp, message, request.url());
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

std::shared_ptr<vds::auth_session> vds::_web_server::get_session(
  const service_provider & sp,
  const std::string & session_id) const {

  std::shared_lock<std::shared_mutex> lock(this->auth_session_mutex_);
  const auto p = this->auth_sessions_.find(session_id);
  if (this->auth_sessions_.end() != p) {
    return p->second;
  }

  return nullptr;
}

void vds::_web_server::kill_session(const service_provider& sp, const std::string& session_id) {
  std::shared_lock<std::shared_mutex> lock(this->auth_session_mutex_);
  const auto p = this->auth_sessions_.find(session_id);
  if (this->auth_sessions_.end() != p) {
    this->auth_sessions_.erase(p);
  }
}

std::shared_ptr<vds::user_manager> vds::_web_server::get_secured_context(
  const service_provider & sp,
  const std::string & session_id) const {

  auto session = this->get_session(sp, session_id);

  if (session) {
    return session->get_secured_context(sp);
  }

  return nullptr;
}

