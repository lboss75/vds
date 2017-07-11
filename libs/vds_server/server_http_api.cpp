/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_http_api.h"
#include "server_http_api_p.h"
#include "http_serializer.h"
#include "http_parser.h"
#include "http_context.h"
#include "server_database_p.h"

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
  : middleware_(*this)
{
}

vds::async_task<> vds::_server_http_api::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  return create_async_task(
    [this, address, port, certificate, private_key](const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
    //upnp_client upnp(sp);
    //upnp.open_port(8000, 8000, "TCP", "VDS Service");

    this->server_.start(
      sp,
      address,
      port,
      [this, certificate, private_key](const service_provider & sp, const tcp_network_socket & s) {

      auto crypto_tunnel = std::make_shared<ssl_tunnel>(false, &certificate, &private_key);

      auto stream = std::make_shared<async_stream<std::shared_ptr<http_message>>>();
      async_series(
        create_async_task(
          [this, s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          read_tcp_network_socket(s, this->cancellation_source_.token()),
          stream_write<uint8_t>(crypto_tunnel->crypted_input())
        )(done, on_error, sp.create_scope("Server SSL Input"));
      }),
        create_async_task(
          [this, s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          stream_read<uint8_t>(crypto_tunnel->crypted_output()),
          write_tcp_network_socket(s, this->cancellation_source_.token())
        )(done, on_error, sp.create_scope("Server SSL Output"));
      }),
        create_async_task(
          [this, s, stream, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          stream_read<uint8_t>(crypto_tunnel->decrypted_output()),
          http_parser(
            [this, stream, crypto_tunnel](const service_provider & sp, const std::shared_ptr<http_message> & request) {
          sp.set_property(
            service_provider::property_scope::local_scope,
            new http_context(crypto_tunnel->get_peer_certificate()));
          this->middleware_.process(sp, request)
            .wait(
              [stream](const service_provider & sp, const std::shared_ptr<http_message> & response) {
            stream->write_all_async(sp, &response, 1)
              .wait(
                [](const service_provider & sp) {},
                [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                  sp.unhandled_exception(ex); 
                },
                sp);
            },
              [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                sp.unhandled_exception(ex);
              },
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
          [crypto_tunnel](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Connection closed");
      },
          [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
      },
        sp);
      crypto_tunnel->start(sp);
    }).wait(done, on_error, sp);
  });
}

void vds::_server_http_api::stop(const service_provider & sp)
{
  this->cancellation_source_.cancel();
}

vds::async_task<std::shared_ptr<vds::http_message>> vds::_server_http_api::route(const service_provider & sp, const std::shared_ptr<http_message>& message) const
{
  http_request request(message);

  if ("/vds/client_api" == request.url()) {
    return create_async_task(
      [this, message](const std::function<void(const service_provider & sp, std::shared_ptr<vds::http_message> response)> & done,
        const error_handler & on_error,
        const service_provider & sp)
    {
      auto json_request = new std::shared_ptr<json_value>();
      dataflow(
        stream_read<uint8_t>(message->body()),
        byte_to_char(),
        json_parser("client_api"),
        dataflow_require_once<std::shared_ptr<json_value>>(json_request)
      )(
        [this, done, json_request](const service_provider & sp) {

          server_database_scope scope(sp);

          done(sp, http_response::simple_text_response(sp, this->server_json_client_api_(scope, *json_request)->str()));
        },
        on_error, sp);
    });
  }

  return http_router::route(sp, message);
}

