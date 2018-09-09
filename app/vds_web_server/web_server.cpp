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
      [sp, pthis, session](const http_message & request) -> std::future<http_message> {
      try {
        if (request.headers().empty()) {
          session->stream_.reset();
          session->handler_.reset();
          return std::future<http_message>::empty();
        }

        std::string keep_alive_header;
        //bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
        return pthis->middleware_.process(sp, request);
      }
      catch (const std::exception & ex) {
        return std::future<http_message>::result(http_response::status_response(sp, http_response::HTTP_Internal_Server_Error, ex.what()));
      }
    });
    session->s_.start(sp, *session->handler_);
  }).execute([sp](const std::shared_ptr<std::exception> & ex) {
    if(ex) {
      sp.get<logger>()->trace(ThisModule, sp, "%s at web server", ex->what());
    }
  });
}

std::future<void> vds::_web_server::prepare_to_stop(const service_provider& sp) {
  return std::future<void>::empty();
}

std::future<vds::http_message> vds::_web_server::route(
  const service_provider& sp,
  const http_message& message) {



  http_request request(message);
  if(request.url() == "/api/channels") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
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
      return std::future<vds::http_message>::result(
        http_response::simple_text_response(
          sp,
          result->str(),
          "application/json; charset=utf-8"));
    }

    if (request.method() == "POST") {
      auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
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

  if (request.url() == "/api/try_login" && request.method() == "GET") {
    const auto login = request.get_parameter("login");
    const auto password = request.get_parameter("password");

    return api_controller::get_login_state(
      sp,
      login,
      password,
      this->shared_from_this(),
      message);
  }

  if (request.url() == "/api/session" && request.method() == "GET") {
    const auto session_id = request.get_parameter("session");

    return api_controller::get_session(
      sp,
      this->shared_from_this(),
      session_id);
  }

  if (request.url() == "/api/logout" && request.method() == "POST") {
    const auto session_id = request.get_parameter("session");

    return api_controller::logout(
      sp,
      this->shared_from_this(),
      session_id);
  }

  if (request.url() == "/api/channel_feed") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
          http_response::status_response(
            sp,
            http_response::HTTP_Unauthorized,
            "Unauthorized"));
      }

      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));

      return api_controller::channel_feed(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id)
        .then([sp](const std::shared_ptr<vds::json_value> & result) {

        return std::future<vds::http_message>::result(
          http_response::simple_text_response(
            sp,
            result->str(),
            "application/json; charset=utf-8"));
      });
    }
  }

  if (request.url() == "/api/devices") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
            http_response::status_response(
                sp,
                http_response::HTTP_Unauthorized,
                "Unauthorized"));
      }

      return api_controller::user_devices(
          sp,
          user_mng,
          this->shared_from_this())
          .then([sp](const std::shared_ptr<vds::json_value> & result) {

            return std::future<vds::http_message>::result(
                http_response::simple_text_response(
                    sp,
                    result->str(),
                    "application/json; charset=utf-8"));
          });
    }
    if (request.method() == "POST") {
      auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
            http_response::status_response(
                sp,
                http_response::HTTP_Unauthorized,
                "Unauthorized"));
      }

      http_request request(message);
      const auto device_name = request.get_parameter("name");
      const auto reserved_size = safe_cast<uint64_t>(std::atoll(request.get_parameter("size").c_str()));
      const auto local_path = request.get_parameter("path");
      return api_controller::lock_device(sp, user_mng, this->shared_from_this(), device_name, local_path,
                                         reserved_size)
          .then([sp]() {

            return std::future<vds::http_message>::result(
                http_response::status_response(
                    sp,
                    http_response::HTTP_OK,
                    "OK"));
          });
    }
  }

  if (request.url() == "/api/offer_device") {
    if (request.method() == "GET") {
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
            http_response::status_response(
                sp,
                http_response::HTTP_Unauthorized,
                "Unauthorized"));
      }

      return api_controller::offer_device(
          sp,
          user_mng,
          this->shared_from_this())
          .then([sp](const std::shared_ptr<vds::json_value> &result) {

            return std::future<vds::http_message>::result(
                http_response::simple_text_response(
                    sp,
                    result->str(),
                    "application/json; charset=utf-8"));
          });
    }
  }

  if (request.url() == "/api/download") {
    if (request.method() == "GET") {
      message.ignore_empty_body();
      const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
      if (!user_mng) {
        return std::future<vds::http_message>::result(
          http_response::status_response(
            sp,
            http_response::HTTP_Unauthorized,
            "Unauthorized"));
      }

      const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));
      const auto file_hash = base64::to_bytes(request.get_parameter("object_id"));

      return api_controller::download_file(
        sp,
        user_mng,
        this->shared_from_this(),
        channel_id,
        file_hash)
        .then([sp](
          const std::string & content_type,
          const std::string & filename,
          size_t body_size,
          const std::shared_ptr<vds::continuous_buffer<uint8_t>> & output_stream) {

        return std::future<vds::http_message>::result(
          http_response::file_response(
            sp,
            output_stream,
            content_type,
            filename,
            body_size));
      });
    }
  }

  if (request.url() == "/api/parse_join_request" && request.method() == "POST") {
    const auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      return std::future<vds::http_message>::result(
        http_response::status_response(
          sp,
          http_response::HTTP_Unauthorized,
          "Unauthorized"));
    }

    return index_page::parse_join_request(
      sp,
      user_mng,
      this->shared_from_this(),
      message);
  }
  if (request.url() == "/approve_join_request" && request.method() == "POST") {
    auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      return std::future<vds::http_message>::result(
        http_response::status_response(
          sp,
          http_response::HTTP_Unauthorized,
          "Unauthorized"));
    }

    return index_page::approve_join_request(
      sp,
      user_mng,
      this->shared_from_this(),
      message);
  }

  if(request.url() == "/upload" && request.method() == "POST") {
    auto user_mng = this->get_secured_context(sp, request.get_parameter("session"));
    if (!user_mng) {
      message.ignore_body();
      return std::future<vds::http_message>::result(
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
    return std::future<vds::http_message>::result(this->router_.route(sp, message, "/index"));
  }

  if (request.url() == "/api/register_requests" && request.method() == "GET") {
    message.ignore_empty_body();
    return api_controller::get_register_requests(sp, this->shared_from_this())
      .then([sp](const std::shared_ptr<vds::json_value> &result) {

      return std::future<vds::http_message>::result(
        http_response::simple_text_response(
          sp,
          result->str(),
          "application/json; charset=utf-8"));
    });
  }
  if (request.url() == "/api/register_request" && request.method() == "GET") {
    message.ignore_empty_body();
    return api_controller::get_register_request(
      sp,
      this->shared_from_this(),
      base64::to_bytes(request.get_parameter("id")))
      .then([sp](const std::shared_ptr<vds::json_value> &result) {

      return std::future<vds::http_message>::result(
        http_response::simple_text_response(
          sp,
          result->str(),
          "application/json; charset=utf-8"));
    });
  }
  if (request.url() == "/api/download_register_request" && request.method() == "GET") {
    const auto request_id = base64::to_bytes(request.get_parameter("id"));
    message.ignore_empty_body();

    return api_controller::get_register_request_body(sp, this->shared_from_this(), request_id)
      .then([sp](const const_data_buffer &result) {

      return std::future<vds::http_message>::result(
        http_response::file_response(
          sp,
          result,
          "user_invite.bin"));
    });
  }

  if(request.url() == "/register_request" && request.method() == "POST") {
    return login_page::register_request_post(sp, this->shared_from_this(), message);
  }

  if (request.url() == "/api/statistics") {
    if (request.method() == "GET") {
      message.ignore_empty_body();

      return api_controller::get_statistics(
        sp,
        this->shared_from_this(),
        message).then([sp](const std::shared_ptr<vds::json_value> & result) {
        return http_response::simple_text_response(
          sp,
          result->str(),
          "application/json; charset=utf-8");
      });
    }
  }

  return std::future<vds::http_message>::result(this->router_.route(sp, message, request.url()));
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

