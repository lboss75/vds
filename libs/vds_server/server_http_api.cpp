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
      auto server = std::make_shared<http_server>();
      async_series(
        server->start(sp,
          crypto_tunnel->decrypted_output(), crypto_tunnel->decrypted_input(),
          [this, crypto_tunnel](
            const vds::service_provider & sp,
            const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

            auto scope = sp.create_scope("Process HTTP Request");
            scope.set_property(
              service_provider::property_scope::local_scope,
              new http_context(crypto_tunnel->get_peer_certificate()));
            return this->middleware_.process(scope, request);
        }),
        create_async_task(
          [this, s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          dataflow(
            stream_read<continuous_stream<uint8_t>>(s.incoming()),
            stream_write<continuous_stream<uint8_t>>(crypto_tunnel->crypted_input())
          )(
            [done](const service_provider & sp) {
            sp.get<logger>()->debug("HTTPAPI", sp, "SSL Input closed");
            done(sp);
          }, on_error, sp.create_scope("Server SSL Input"));
        }),
        create_async_task(
          [this, s, crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
          dataflow(
            stream_read(crypto_tunnel->crypted_output()),
            stream_write<continuous_stream<uint8_t>>(s.outgoing())
          )([done](const service_provider & sp) {
            sp.get<logger>()->debug("HTTPAPI", sp, "SSL Output closed");
            done(sp);
          }, on_error, sp.create_scope("Server SSL Output"));
        })
      ).wait(
        [crypto_tunnel, server](const service_provider & sp) {
          sp.get<logger>()->debug("HTTPAPI", sp, "Connection closed");
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
        stream_read<continuous_stream<uint8_t>>(message->body()),
        byte_to_char(),
        json_parser("client_api"),
        dataflow_require_once<std::shared_ptr<json_value>>(json_request)
      )(
        [this, done, json_request](const service_provider & sp) {

          //database_transaction_scope scope(sp, *(*sp.get<iserver_database>())->get_db());
          done(sp, http_response::simple_text_response(sp, this->server_json_client_api_(
            sp,
            *json_request)->str()));
          //scope.commit();
        },
        on_error, sp);
    });
  }

  return http_router::route(sp, message);
}

