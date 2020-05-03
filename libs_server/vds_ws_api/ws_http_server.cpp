/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/api_controller.h"
#include "ws_http_server.h"
#include "private/ws_http_server_p.h"
#include "http_parser.h"
#include "tcp_network_socket.h"
#include "http_serializer.h"
#include "http_multipart_reader.h"
#include "http_pipeline.h"
#include "user_manager.h"
#include "private/auth_session.h"
#include "private/index_page.h"
#include "db_model.h"
#include "websocket_api.h"

vds::ws_http_server::ws_http_server()
: port_(8050) {
}

vds::ws_http_server::~ws_http_server() {
}

vds::expected<void> vds::ws_http_server::register_services(service_registrator&) {
  return expected<void>();
}

vds::expected<void> vds::ws_http_server::start(const service_provider * sp) {
  this->impl_ = std::make_shared<_ws_http_server>(sp);
  return this->impl_->start(this->port_).get();
}

vds::expected<void> vds::ws_http_server::stop() {
  this->impl_.reset();

  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::ws_http_server::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

vds::_ws_http_server * vds::ws_http_server::operator->() const {
  return this->impl_.get();
}
/////////////////////////////////////////////////////////////
vds::_ws_http_server::_ws_http_server(const service_provider * sp)
: sp_(sp),
router_({
  {"/api/ws", "GET", &websocket_api::open_connection }
  })
{
}

vds::_ws_http_server::~_ws_http_server() {
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::_ws_http_server::process_message(
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<session_data> & session,
  vds::http_message request) {
  if (request.headers().empty()) {
    this->sp_->get<vds::logger>()->debug(ThisModule, "Route end");
    session->stream_.reset();
    session->handler_.reset();
    return expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>();
  }

  this->sp_->get<vds::logger>()->debug(ThisModule, "Route [%s]", request.headers().front().c_str());

  return this->router_.route(this->sp_, output_stream, request);
}

vds::async_task<vds::expected<void>> vds::_ws_http_server::start(
    uint16_t port) {
  CHECK_EXPECTED_ASYNC(this->server_.start(this->sp_, network_address::any_ip4(port),
    [sp = this->sp_, pthis = this->shared_from_this()](std::shared_ptr<tcp_network_socket> s) -> vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>{
    GET_EXPECTED_ASYNC(writer, s->get_output_stream(sp));
    auto session = std::make_shared<session_data>(sp, writer);
    session->handler_ = std::make_shared<http_pipeline>(
		    sp,
        session->stream_,
      [sp, pthis, session](http_message request) -> vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> {
      return pthis->process_message(session->stream_, session, std::move(request));
    });
    co_return session->handler_;
  }));


  co_return vds::expected<void>();
}

vds::async_task<vds::expected<void>> vds::_ws_http_server::prepare_to_stop() {
	return expected<void>();
}
