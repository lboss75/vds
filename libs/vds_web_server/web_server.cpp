/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "web_server.h"
#include "private/web_server_p.h"
#include "http_parser.h"
#include "tcp_network_socket.h"
#include "http_serializer.h"
#include "../vds_file_manager/stdafx.h"
#include "http_multipart_reader.h"

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
  this->impl_->start(scope);
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

  this->router_.add_static(
    "/",
    "<html><body>Hello World</body></html>");

}

vds::_web_server::~_web_server() {
}

struct session_data {
  vds::tcp_network_socket s_;
  std::shared_ptr<vds::http_async_serializer> stream_;
  std::shared_ptr<vds::http_parser> handler_;

  session_data(vds::tcp_network_socket && s)
    : s_(std::move(s)),
      stream_(std::make_shared<vds::http_async_serializer>(this->s_)) {
  }
};

void vds::_web_server::start(const service_provider& sp) {
  this->server_.start(sp, network_address::any_ip4(8050), [sp, pthis = this->shared_from_this()](tcp_network_socket s) {
    auto session = std::make_shared<session_data>(std::move(s));
    session->handler_ = std::make_shared<http_parser>(sp, [sp, pthis, session](const http_message & request) -> async_task<> {
      if(request.headers().empty()) {
        session->stream_.reset();
        session->handler_.reset();
        return async_task<>::empty();
      }

      std::string keep_alive_header;
      bool keep_alive = request.get_header("Connection", keep_alive_header) && keep_alive_header == "Keep-Alive";
      return pthis->middleware_.process(sp, request, session->stream_)
      .then([session, sp, keep_alive](const http_message & response) {
        return session->stream_->write_async(sp, response).then([sp, session, keep_alive]() -> async_task<> {
          if(!keep_alive) {
            return session->stream_->write_async(sp, http_message());//Close session
          }
          return async_task<>::empty();
        });
      });
    },
      [session, sp]() {
      barrier b;
      auto continue_message = http_response(100, "Continue").create_message(sp);
      return session->stream_->write_async(sp, continue_message)
        .then([continue_message]() {
        continue_message.body()->write_async(nullptr, 0);
      }).execute([&b](const std::shared_ptr<std::exception> & ex) {
        b.set();
      });

      b.wait();

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

class file_upload_task : public std::enable_shared_from_this<file_upload_task> {
public:
  vds::async_task<> read_part(const vds::http_message& part) {
    return part.body()->read_async(this->buffer_, sizeof(this->buffer_)).then([pthis = this->shared_from_this(), part](size_t readed) -> vds::async_task<> {
      if(0 == readed) {
        return vds::async_task<>::empty();
      }

      return pthis->read_part(part);
    });
  }

  vds::http_message get_response(const vds::service_provider& sp) {
    return vds::http_response::simple_text_response(sp, std::string());
  }
private:
  uint8_t buffer_[1024];
};

vds::async_task<vds::http_message> vds::_web_server::route(
  const service_provider& sp,
  const http_message& message,
  const std::shared_ptr<http_async_serializer> & output) const {

  http_request request(message);
  if(request.url() == "/upload/" && request.method() == "POST") {
    std::string content_type;
    if(request.get_header("Content-Type", content_type)) {
      static const char multipart_form_data[] = "multipart/form-data;";
      if(multipart_form_data == content_type.substr(0, sizeof(multipart_form_data) - 1)) {
        auto boundary = content_type.substr(sizeof(multipart_form_data) - 1);
        trim(boundary);
        static const char boundary_prefix[] = "boundary=";
        if (boundary_prefix == boundary.substr(0, sizeof(boundary_prefix) - 1)) {
          boundary.erase(0, sizeof(boundary_prefix) - 1);

          auto task = std::make_shared<file_upload_task>();
          auto reader = std::make_shared<http_multipart_reader>(sp, "--" + boundary, [task](const http_message& part)->async_task<> {
            return task->read_part(part);
          });

          //std::string expect_value;
          //if (request.get_header("Expect", expect_value) && "100-continue" == expect_value) {
          //  auto continue_message = http_response(100, "Continue").create_message(sp);
          //  return output->write_async(sp, continue_message)
          //    .then([continue_message]() {
          //    return continue_message.body()->write_async(nullptr, 0);
          //  }).then([sp, task]() {
          //    return task->get_response(sp);
          //  });
          //}

          return reader->start(sp, message).then([sp, task, output, request]() ->async_task<http_message> {
            return async_task<http_message>::result(task->get_response(sp));
          });
        }
      }
    }
  }

  return vds::async_task<vds::http_message>::result(this->router_.route(sp, message));
}

