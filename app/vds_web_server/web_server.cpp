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
#include "storage_api.h"

vds::web_server::web_server()
: port_(8050) {
}

vds::web_server::~web_server() {
}

void vds::web_server::register_services(service_registrator&) {
}

void vds::web_server::start(const service_provider * sp) {
  this->impl_ = std::make_shared<_web_server>(sp);
  this->impl_->start(this->root_folder_, this->port_).get();
}

void vds::web_server::stop() {
  this->impl_.reset();
}

vds::async_task<void> vds::web_server::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

vds::_web_server * vds::web_server::operator->() const {
  return this->impl_.get();
}
/////////////////////////////////////////////////////////////
vds::_web_server::_web_server(const service_provider * sp)
: sp_(sp),
router_({
  { "/api/channels", "GET",  &api_controller::get_channels },
  { "/api/channels", "POST", &index_page::create_channel },
  { "/api/try_login", "GET", [this](
    const vds::service_provider * sp,
    const http_request & request) -> async_task<std::shared_ptr<json_value>> {
      const auto login = request.get_parameter("login");
      const auto password = request.get_parameter("password");

      co_await request.get_message().ignore_empty_body();

      co_return co_await api_controller::get_login_state(
        sp,
        login,
        password,
        this->shared_from_this(),
        request);
  }
    },
  { "/api/login", "GET", [this](
    const vds::service_provider * sp,
    const http_request & request) -> async_task<std::shared_ptr<json_value>> {
      const auto login = request.get_parameter("login");
      const auto password = request.get_parameter("password");

      co_await request.get_message().ignore_empty_body();

      co_return co_await api_controller::login(
        sp,
        login,
        password,
        this->shared_from_this(),
        request);
  }
    },
  {"/api/session", "GET", [this](
    const vds::service_provider * sp,
    const http_request & request) -> async_task<std::shared_ptr<json_value>> {
    const auto session_id = request.get_parameter("session");

    co_await request.get_message().ignore_empty_body();

    co_return co_await api_controller::get_session(
      this->get_session(session_id));
    }
  },
  {"/api/logout", "POST", [this](
    const vds::service_provider * sp,
    const http_request & request) -> async_task<http_message> {
    const auto session_id = request.get_parameter("session");

    co_await request.get_message().ignore_empty_body();

      this->kill_session(session_id);

    co_return http_response::redirect("/");
    }
  },
  {"/api/channel_feed", "GET", [this](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_request & request) -> async_task<std::shared_ptr<json_value>> {
            const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));

            co_await request.get_message().ignore_empty_body();
            
      co_return co_await api_controller::channel_feed(
              sp,
              user_mng,
              channel_id);
    }
  },
  {"/api/devices", "GET", &storage_api::device_storages },
  {"/api/devices", "POST", [](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_request & request) -> async_task<http_message> {
            const auto name = request.get_parameter("name");
            const auto reserved_size = safe_cast<uint64_t>(std::atoll(request.get_parameter("size").c_str()));
            const auto local_path = request.get_parameter("path");
      
      co_await request.get_message().ignore_empty_body();

      co_await storage_api::add_device_storage(
              sp,
              user_mng,
              name,
              local_path,
              reserved_size);

            co_return http_response::status_response(
                          http_response::HTTP_OK,
                          "OK");
          }
  },
  {"/api/download", "GET", [](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_request & request) -> async_task<http_message> {
                  const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));
                  const auto file_hash = base64::to_bytes(request.get_parameter("object_id"));

                  auto buffer = std::make_shared<continuous_buffer<uint8_t>>(sp);

      co_await request.get_message().ignore_empty_body();

                  auto result = co_await api_controller::download_file(
                    sp,
                    user_mng,
                    channel_id,
                    file_hash,
                    std::make_shared<continuous_stream_output_async<uint8_t>>(buffer));

                    co_return http_response::file_response(
                        std::make_shared<continuous_stream_input_async<uint8_t>>(buffer),
                        result.size,
                        result.name,
                        result.mime_type);
                      }
  },
  {"/api/prepare_download", "GET", [](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_request & request) -> async_task<std::shared_ptr<json_value>> {
                  const auto channel_id = base64::to_bytes(request.get_parameter("channel_id"));
                  const auto file_hash = base64::to_bytes(request.get_parameter("object_id"));

      co_await request.get_message().ignore_empty_body();

                  co_return co_await api_controller::prepare_download_file(
                    sp,
                    user_mng,
                    channel_id,
                    file_hash);
      }
  },
  {"/api/parse_join_request", "POST", &index_page::parse_join_request },
  {"/api/approve_join_request", "POST", &index_page::approve_join_request },
  {"/upload", "POST", &index_page::create_message },
  {"/api/statistics", "GET", &api_controller::get_statistics },
  })
{
  this->router_.auth_callback([this](const http_request & message) {
    return this->get_secured_context(message.get_parameter("session"));
  });
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

vds::async_task<void> vds::_web_server::start(
    const std::string & root_folder,
    uint16_t port) {
  this->load_web("/", foldername(root_folder));
  co_await this->server_.start(this->sp_, network_address::any_ip4(port),
    [sp = this->sp_, pthis = this->shared_from_this()](std::shared_ptr<tcp_network_socket> s) -> vds::async_task<void>{
    auto[reader, writer] = s->start(sp);
    auto session = std::make_shared<session_data>(writer);
    session->handler_ = std::make_shared<http_pipeline>(
        session->stream_,
      [sp, pthis, session](const http_message request) -> vds::async_task<http_message> {
      try {
        if (request.headers().empty()) {
          session->stream_.reset();
          session->handler_.reset();
          co_return http_message();
        }

        std::string keep_alive_header;
        //bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
        co_return co_await pthis->router_.route(sp, request);
      }
      catch (const std::exception & ex) {
        co_return http_response::status_response(http_response::HTTP_Internal_Server_Error, ex.what());
      }
    });
    co_await session->handler_->process(reader);
  });
}

vds::async_task<void> vds::_web_server::prepare_to_stop() {
  co_return;
}

//  if (request.url() == "/api/offer_device") {
//    if (request.method() == "GET") {
//      const auto user_mng = this->get_secured_context(request.get_parameter("session"));
//      if (!user_mng) {
//        co_return http_response::status_response(
//                http_response::HTTP_Unauthorized,
//                "Unauthorized");
//      }
//
//      auto result = co_await api_controller::offer_device(
//          this->sp_,
//          user_mng,
//          this->shared_from_this());
//
//            co_return http_response::simple_text_response(
//                    result->str(),
//                    "application/json; charset=utf-8");
//    }
//  }
//

//
//  if (request.url() == "/api/download_register_request" && request.method() == "GET") {
//    const auto request_id = base64::to_bytes(request.get_parameter("id"));
//    co_await message.ignore_empty_body();
//
//    auto result = co_await api_controller::get_register_request_body(
//        this->sp_,
//        this->shared_from_this(),
//        request_id);
//    co_return http_response::file_response(
//        result,
//        "user_invite.bin");
//  }
//
//  if(request.url() == "/register_request" && request.method() == "POST") {
//    co_return co_await login_page::register_request_post(
//        this->sp_,
//        this->shared_from_this(),
//        message);
//  }
//
//
//  co_return co_await this->router_.route(message, request.url());
//}

void vds::_web_server::load_web(const std::string& path, const foldername & folder) {
  foldername f(folder);
  f.files([this, path](const filename & fn) -> bool{
    if(".html" == fn.extension()) {
      if(fn.name_without_extension() == "index") {
        this->router_.add_file(path, fn);
      }
      else {
        this->router_.add_file(path + fn.name_without_extension(), fn);
      }
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
  
  const std::string & session_id) const {

  std::shared_lock<std::shared_mutex> lock(this->auth_session_mutex_);
  const auto p = this->auth_sessions_.find(session_id);
  if (this->auth_sessions_.end() != p) {
    return p->second;
  }

  return nullptr;
}

void vds::_web_server::kill_session( const std::string& session_id) {
  std::shared_lock<std::shared_mutex> lock(this->auth_session_mutex_);
  const auto p = this->auth_sessions_.find(session_id);
  if (this->auth_sessions_.end() != p) {
    this->auth_sessions_.erase(p);
  }
}

std::shared_ptr<vds::user_manager> vds::_web_server::get_secured_context(
  
  const std::string & session_id) const {

  auto session = this->get_session(session_id);

  if (session) {
    return session->get_secured_context();
  }

  return nullptr;
}

