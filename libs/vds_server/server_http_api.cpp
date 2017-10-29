/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <iostream>
#include <ctime>
#include <iomanip> // put_time
#include "server.h"
#include "server_http_api.h"
#include "private/server_http_api_p.h"
#include "http_serializer.h"
#include "http_parser.h"
#include "http_context.h"
#include "private/server_database_p.h"

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
  return [this, sp, address, port, certificate, private_key]() {
    //upnp_client upnp(sp);
    //upnp.open_port(8000, 8000, "TCP", "VDS Service");

    return this->server_.start(
      sp,
      address,
      port,
      [this, certificate, private_key](const service_provider & sp, const tcp_network_socket & s) {

      auto crypto_tunnel = std::make_shared<ssl_tunnel>(false, &certificate, &private_key);
      auto server = std::make_shared<http_server>();
      async_series(
        server->start(sp,
          crypto_tunnel->decrypted_output(), crypto_tunnel->decrypted_input(),
          [this, sp, crypto_tunnel](const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

            auto scope = sp.create_scope("Process HTTP Request");
            scope.set_property(
              service_provider::property_scope::local_scope,
              new http_context(crypto_tunnel->get_peer_certificate()));
            return this->middleware_.process(scope, request);
        }),
        copy_stream(
          sp,
          s.incoming(),
          crypto_tunnel->crypted_input()
        ),
        copy_stream(
          sp,
          crypto_tunnel->crypted_output(),
          s.outgoing()
        )
      ).execute(
        [sp, crypto_tunnel, server](const std::shared_ptr<std::exception> & ex) {
          if(!ex){
            sp.get<logger>()->debug("HTTPAPI", sp, "Connection closed");
          } else {
            sp.unhandled_exception(ex);
        };
      crypto_tunnel->start(sp);
    });
  });
};
}

void vds::_server_http_api::stop(const service_provider & /*sp*/)
{
}

vds::async_task<std::shared_ptr<vds::http_message>> vds::_server_http_api::route(const service_provider & sp, const std::shared_ptr<http_message>& message) const
{
  http_request request(message);

//   if ("/vds/client_api" == request.url()) {
//     auto json_request = new std::shared_ptr<json_value>();
//     return dataflow(
//         stream_read<continuous_buffer<uint8_t>>(message->body()),
//         byte_to_char(),
//         json_parser("client_api"),
//         dataflow_require_once<std::shared_ptr<json_value>>(json_request)
//       )
//     .then(
//         [this, json_request](
//           const std::function<void(const service_provider & sp, std::shared_ptr<vds::http_message> response)> & done,
//           const error_handler & on_error,
//           const service_provider & sp) {
// 
//           done(
//             sp,
//             http_response::simple_text_response(sp, this->server_json_client_api_(
//               sp,
//               *json_request)->str()));
//           });
//   }
/*
  if ("/vds/dump_state" == request.url()) {
    return [this, sp, message](const std::function<void(const service_provider & sp, std::shared_ptr<vds::http_message> response)> & done,
        const error_handler & on_error,
        const service_provider & sp)
    {
      (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
        [this, sp, done, on_error](database_transaction & t) -> bool {
          auto result = std::make_shared<json_object>();

          auto now = std::chrono::system_clock::now();
          auto in_time_t = std::chrono::system_clock::to_time_t(now);

          std::stringstream ss;
          ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");

          result->add_property("timestamp", ss.str());

          sp.get<ichunk_manager>()->dump_state(sp, t, result);

          done(sp, http_response::simple_text_response(sp, static_cast<const json_value *>(result.get())->str()));

          return true;
      });
    });
  }*/

  return http_router::route(sp, message);
}

