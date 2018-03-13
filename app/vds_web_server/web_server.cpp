/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <queue>
#include <private/file_upload_task_p.h>
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

vds::web_server::web_server() {
}

vds::web_server::~web_server() {
}

void vds::web_server::register_services(service_registrator&) {
}

void vds::web_server::start(const service_provider& sp) {
  auto scope = sp.create_scope("Web server");
  mt_service::enable_async(scope);
  this->impl_ = std::make_shared<_web_server>(scope);
  this->impl_->start(scope, this->root_folder_);
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

void vds::_web_server::start(const service_provider& sp, const std::string & root_folder) {
  this->load_web("/", foldername(root_folder));
  this->server_.start(sp, network_address::any_ip4(8050), [sp, pthis = this->shared_from_this()](tcp_network_socket s) {
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
  const http_message& message) const {

  http_request request(message);
  if(request.url() == "/api/channels" && request.method() == "GET") {
    auto result = std::make_shared<json_array>();
    auto user_mng = sp.get<user_manager>();
    for(auto & channel : user_mng->get_channels()) {
      auto item = std::make_shared<json_object>();
      item->add_property("id", channel.id().str());
      item->add_property("name", channel.name());
      result->add(item);
    }

    return vds::async_task<vds::http_message>::result(
      http_response::simple_text_response(sp, result->json_value::str(), "application/json; charset=utf-8"));
  }
  if(request.url() == "/upload" && request.method() == "POST") {
    std::string content_type;
    if(request.get_header("Content-Type", content_type)) {
      static const char multipart_form_data[] = "multipart/form-data;";
      if(multipart_form_data == content_type.substr(0, sizeof(multipart_form_data) - 1)) {
        auto boundary = content_type.substr(sizeof(multipart_form_data) - 1);
        trim(boundary);
        static const char boundary_prefix[] = "boundary=";
        if (boundary_prefix == boundary.substr(0, sizeof(boundary_prefix) - 1)) {
          boundary.erase(0, sizeof(boundary_prefix) - 1);

          auto task = std::make_shared<_file_upload_task>();
          auto reader = std::make_shared<http_multipart_reader>(sp, "--" + boundary, [sp, task](const http_message& part)->async_task<> {
            return task->read_part(sp, part);
          });

          return reader->process(sp, message).then([sp, task](){
            return task->get_response(sp);
          });
        }
      }
    }
  }

  return vds::async_task<vds::http_message>::result(this->router_.route(sp, message));
}

void vds::_web_server::load_web(const std::string& path, const foldername & folder) {
  foldername f(folder);
  f.files([this, path](const filename & fn) -> bool{
    this->router_.add_file(path + fn.name(), fn);
    return true;
  });
  f.folders([this, path](const foldername & fn) -> bool {
    this->load_web(path + fn.name() + "/", fn);
    return true;
  });
}

