/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_http_api.h"
#include "server_http_api_p.h"


vds::async_task<> vds::server_http_api::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  return static_cast<_server_http_api *>(this)->start(
    sp, address, port, certificate, private_key);
}

/////////////////////////////

vds::_server_http_api::_server_http_api()
{
}

vds::async_task<> vds::_server_http_api::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  //upnp_client upnp(sp);
  //upnp.open_port(8000, 8000, "TCP", "VDS Service");

  this->server_.start(
    sp,
    address,
    port,
    [this, certificate, private_key](const service_provider & sp, const tcp_network_socket & s) {

    auto crypto_tunnel = std::make_shared<ssl_tunnel>(false, &certificate, &private_key);

    auto responses = new std::shared_ptr<http_message>[1];
    auto stream = std::make_shared<async_stream<std::shared_ptr<http_message>>>();
    async_series(
      create_async_task(
        [s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          dataflow(
            read_tcp_network_socket(s),
            stream_write<uint8_t>(crypto_tunnel->crypted_input())
          )(done, on_error, sp.create_scope("Server SSL Input"));
        }),
      create_async_task(
        [s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          dataflow(
            stream_read<uint8_t>(crypto_tunnel->crypted_output()),
            write_tcp_network_socket(s)
          )(done, on_error, sp.create_scope("Server SSL Output"));
        }),
      create_async_task(
        [this, s, stream, responses, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          dataflow(
            stream_read<uint8_t>(crypto_tunnel->decrypted_output()),
            http_parser(
          [this, stream, &router, responses](const service_provider & sp, const std::shared_ptr<http_message> & request) {
            responses[0] = this->middleware_.process(sp, request);
            stream->write_all_async(sp, responses, 1)
            .wait(
              [](const service_provider & sp) {},
              [](const service_provider & sp, std::exception_ptr ex) {},
              sp);
          }
        )
      )(done, on_error, sp);
    }),
      create_async_task(
        [s, stream, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
      dataflow(
        stream_read<std::shared_ptr<http_message>>(stream),
        http_serializer(),
        stream_write<uint8_t>(crypto_tunnel->decrypted_input())
      )(done, on_error, sp);
    })
      )
      .wait(
        [responses, crypto_tunnel](const service_provider & sp) {
      sp.get<logger>()->debug(sp, "Connection closed");
      delete responses;
    },
        [responses](const service_provider & sp, std::exception_ptr ex) {
      delete responses;
      FAIL() << exception_what(ex);

    },
      sp);
    crypto_tunnel->start(sp);
  }).wait(
    [&b](const service_provider & sp) {
    sp.get<logger>()->debug(sp, "Server has been started");
    b.set();
  },
    [&b](const service_provider & sp, std::exception_ptr ex) {
    FAIL() << exception_what(ex);
    b.set();
  },
    sp
    );

}

void vds::_server_http_api::stop(const service_provider & sp)
{
}

std::shared_ptr<vds::http_message> vds::_server_http_api::route(const service_provider & sp, const std::shared_ptr<http_message>& message) const
{
  http_request request(message);

  if ("/vds/client_api" == request.url()) {
    dataflow(
      stream_read<uint8_t>(message->body()),
      json_parser("client_api"),
      dataflow_for_each([]() {
      })
      )(

        );
  }


  return http_router::route(sp, message);
}

vds::_server_http_api::socket_session::socket_session(
  const http_router & router,
  const certificate & certificate,
  const asymmetric_private_key & private_key)
  : router_(router), certificate_(certificate),
  private_key_(private_key)
{
}

vds::_server_http_api::socket_session::handler::handler(
  const socket_session & owner,
  vds::network_socket & s)
: s_(std::move(s)),
  tunnel_(false, &owner.certificate_, &owner.private_key_),
  certificate_(owner.certificate_),
  private_key_(owner.private_key_),
  server_http_handler_(owner.router_),
  http_server_done_([this](const service_provider & sp) {}),
  http_server_error_([this](const service_provider & sp, std::exception_ptr) {})
{
}

void vds::_server_http_api::socket_session::handler::start(const service_provider & sp)
{
  vds::dataflow(
    input_network_stream(this->s_),
    ssl_input_stream(this->tunnel_),
    http_parser(sp),
    http_middleware<server_http_handler>(this->server_http_handler_),
    http_response_serializer(),
    ssl_output_stream(this->tunnel_),
    output_network_stream(this->s_)
  )
  (
    [this](const service_provider & sp) { delete this; },
    [this](const service_provider & sp, std::exception_ptr) {delete this; },
    sp);
}

vds::_server_http_api::server_http_handler::server_http_handler(
  const http_router & router)
: router_(router)
{  
}

