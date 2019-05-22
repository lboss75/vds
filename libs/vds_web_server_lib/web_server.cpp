/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/api_controller.h"
#include "web_server.h"
#include "private/web_server_p.h"
#include "http_parser.h"
#include "tcp_network_socket.h"
#include "http_serializer.h"
#include "http_multipart_reader.h"
#include "file_operations.h"
#include "http_pipeline.h"
#include "user_manager.h"
#include "private/auth_session.h"
#include "private/index_page.h"
#include "storage_api.h"
#include "db_model.h"
#include "websocket_api.h"

vds::web_server::web_server()
: port_(8050) {
}

vds::web_server::~web_server() {
}

vds::expected<void> vds::web_server::register_services(service_registrator&) {
  return expected<void>();
}

vds::expected<void> vds::web_server::start(const service_provider * sp) {
  this->impl_ = std::make_shared<_web_server>(sp);
  return this->impl_->start(this->port_).get();
}

vds::expected<void> vds::web_server::stop() {
  this->impl_.reset();

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::web_server::prepare_to_stop() {
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
    const http_message & request) -> async_task<expected<std::shared_ptr<json_value>>> {
      const auto login = request.get_parameter("login");
      const auto password = request.get_parameter("password");

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
    const http_message & request) -> async_task<expected<std::shared_ptr<json_value>>> {
      const auto login = request.get_parameter("login");
      const auto password = request.get_parameter("password");

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
    const http_message & request) -> async_task<expected<std::shared_ptr<json_value>>> {
    const auto session_id = request.get_parameter("session");

    co_return co_await api_controller::get_session(
      this->get_session(session_id));
    }
  },
  {"/api/logout", "POST", [this](
    const vds::service_provider * sp,
    const std::shared_ptr<http_async_serializer> & output_stream,
    const http_message & request) -> async_task<expected<void>> {
    const auto session_id = request.get_parameter("session");

    this->kill_session(session_id);

    return http_response::redirect(output_stream, "/");
    }
  },
  {"/api/channel_feed", "GET", [](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_message & request) -> async_task<expected<std::shared_ptr<json_value>>> {
            GET_EXPECTED_ASYNC(channel_id, base64::to_bytes(request.get_parameter("channel_id")));

    std::shared_ptr<json_value> result;
    GET_EXPECTED_VALUE_ASYNC(result, co_await api_controller::channel_feed(
              sp,
              user_mng,
              channel_id));
    co_return result;
    }
  },
  {"/api/devices", "GET", &storage_api::device_storages },
  {"/api/devices", "POST", [](
    const vds::service_provider * sp,
    const std::shared_ptr<http_async_serializer> & output_stream,
    const std::shared_ptr<user_manager> & user_mng,
    const http_message & request) -> async_task<expected<void>> {
            const auto name = request.get_parameter("name");
            const auto reserved_size = safe_cast<uint64_t>(std::atoll(request.get_parameter("size").c_str()));
            const auto local_path = request.get_parameter("path");
      
            CHECK_EXPECTED_ASYNC(co_await storage_api::add_device_storage(
              sp,
              user_mng,
              name,
              local_path,
              reserved_size));

            co_return http_response::status_response(
              output_stream,
              http_response::HTTP_OK,
              "OK");
          }
  },
  {"/api/download", "GET", [](
    const vds::service_provider * sp,
    const std::shared_ptr<http_async_serializer> & output_stream,
    const std::shared_ptr<user_manager> & user_mng,
    const http_message & request) -> async_task<expected<void>> {
                  GET_EXPECTED_ASYNC(channel_id, base64::to_bytes(request.get_parameter("channel_id")));
                  GET_EXPECTED_ASYNC(file_hash, base64::to_bytes(request.get_parameter("object_id")));
                  auto file_name = request.get_parameter("file_name");

                  file_manager::file_operations::download_result_t result;
                  GET_EXPECTED_VALUE_ASYNC(result, co_await api_controller::download_file(
                    sp,
                    user_mng,
                    channel_id,
                    file_name,
                    file_hash));

                    co_return http_response::file_response(
                        output_stream,
                        result.body,
                        result.size,
                        result.name,
                        result.file_hash,
                        result.mime_type);
                      }
  },
  {"/api/prepare_download", "GET", [](
    const vds::service_provider * sp,
    const std::shared_ptr<user_manager> & user_mng,
    const http_message & request) -> async_task<expected<std::shared_ptr<json_value>>> {
                  GET_EXPECTED_ASYNC(channel_id, base64::to_bytes(request.get_parameter("channel_id")));
                  GET_EXPECTED_ASYNC(file_hash, base64::to_bytes(request.get_parameter("object_id")));
                  auto file_name = request.get_parameter("file_name");

                  co_return co_await api_controller::prepare_download_file(
                    sp,
                    user_mng,
                    channel_id,
                    file_name,
                    file_hash);
      }
  },
  {"/api/parse_join_request", "POST", &index_page::parse_join_request },
  {"/api/approve_join_request", "POST", &index_page::approve_join_request },
  {"/api/upload", "POST", &api_controller::create_message },
  {"/upload", "POST", &index_page::create_message },
  {"/api/statistics", "GET", &api_controller::get_statistics },
  {"/api/ws", "GET", &websocket_api::open_connection }
  }),
  update_timer_("web session timer")
{
  this->router_.auth_callback([this](const http_message & message) {
    return this->get_secured_context(message.get_parameter("session"));
  });
  this->router_.not_found_handler([this](
    const std::shared_ptr<http_async_serializer> & output_stream,
    const http_message & message) -> async_task<expected<bool>> {
    return this->not_found_handler(output_stream, message);
  });
}

vds::_web_server::~_web_server() {
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::_web_server::process_message(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<session_data> & session,
  vds::http_message && request) {
  if (request.headers().empty()) {
    this->sp_->get<vds::logger>()->debug(ThisModule, "Route end");
    session->stream_.reset();
    session->handler_.reset();
    return std::shared_ptr<vds::stream_output_async<uint8_t>>();
  }

  this->sp_->get<vds::logger>()->debug(ThisModule, "Route [%s]", request.headers().front().c_str());

  //std::string keep_alive_header;
  //bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
  return this->router_.route(this->sp_, output_stream, request);
}

vds::async_task<vds::expected<void>> vds::_web_server::start(
    uint16_t port) {
  this->web_task_ = this->server_.start(this->sp_, network_address::any_ip4(port),
    [sp = this->sp_, pthis = this->shared_from_this()](const std::shared_ptr<tcp_network_socket> & s) -> vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>{
    GET_EXPECTED_ASYNC(reader, s->get_input_stream(sp));
    GET_EXPECTED_ASYNC(writer, s->get_output_stream(sp));
    auto session = std::make_shared<session_data>(writer);
    session->handler_ = std::make_shared<http_pipeline>(
		    sp,
        session->stream_,
      [sp, pthis, session](http_message && request) -> vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> {
      return pthis->process_message(session->stream_, session, std::move(request));
    });
    CHECK_EXPECTED_ASYNC(co_await session->handler_->process(reader));
    co_return expected<void>();
  });

  CHECK_EXPECTED_ASYNC(this->update_timer_.start(
    this->sp_, std::chrono::minutes(1), [pthis = this->shared_from_this()]()->async_task<expected<bool>> {
    std::set<std::string> to_kill;

    const auto dest_point = std::chrono::steady_clock::now() - std::chrono::minutes(30);

    std::shared_lock<std::shared_mutex> lock(pthis->auth_session_mutex_);
    for(auto p : pthis->auth_sessions_) {
      if(p.second->last_update() < dest_point) {
        to_kill.emplace(p.first);
      }
    }
    lock.unlock();

    for(auto session : to_kill) {
      pthis->kill_session(session);
    }

    co_return !pthis->sp_->get_shutdown_event().is_shuting_down();
  }));

  this->www_user_mng_ = std::make_shared<user_manager>(this->sp_);
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<vds::db_model>()->async_read_transaction([this](database_read_transaction & t)->expected<void> {
    CHECK_EXPECTED(this->www_user_mng_->load(t, cert_control::web_login(), cert_control::web_password()));
    return expected<void>();
  }));

    this->www_channel_ = std::make_shared<file_manager::files_channel>(this->sp_, this->www_user_mng_, cert_control::get_web_channel_id());

  co_return vds::expected<void>();
}

vds::async_task<vds::expected<void>> vds::_web_server::prepare_to_stop() {
	if (this->web_task_.has_state()) {
		return std::move(this->web_task_);
	}

	return expected<void>();
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

vds::async_task<vds::expected<vds::http_message>> vds::_web_server::not_found_handler(
  const http_message& request) {
  if (request.method() == "GET") {
    auto buffer = std::make_shared<continuous_buffer<uint8_t>>(this->sp_);

    CHECK_EXPECTED_ASYNC(co_await request.get_message().ignore_empty_body());

    file_manager::file_operations::download_result_t result;
    GET_EXPECTED_VALUE_ASYNC(result, co_await this->www_channel_->download_file(
      filename(request.url()),
      const_data_buffer(),
      std::make_shared<continuous_stream_output_async<uint8_t>>(buffer)));

    if (result.file_hash.size() > 0) {
      auto content_type = result.mime_type;
      if (content_type.empty()) {
        content_type = "application/octet-stream";
      }
      co_return http_response::simple_text_response(
        std::make_shared<continuous_stream_input_async<uint8_t>>(buffer),
        result.size,
        result.mime_type);
    }
  }

  co_return vds::expected<vds::http_message>();
}

        std::shared_ptr<vds::user_manager> vds::_web_server::get_secured_context(
  
  const std::string & session_id) const {

  auto session = this->get_session(session_id);

  if (session) {
    return session->get_secured_context();
  }

  return nullptr;
}

