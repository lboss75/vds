/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"

vds::background_app::background_app()
  :
  start_server_command_set_("Server start", "Start web server", "start", "server"),
  http_server_done_([this]() { this->http_server_closed(); }),
  http_server_error_([this](std::exception * ex) { this->http_server_error(ex); })
{
  this->router_.add_static(
    "/",
    "<html><body>Hello World</body></html>");
}

void vds::background_app::main(const service_provider & sp)
{
  this->certificate_.load(filename(foldername(persistence::current_user(), ".vds"), "cacert.pem"));
  this->private_key_.load(filename(foldername(persistence::current_user(), ".vds"), "cakey.pem"));

  sequence(
    socket_server(sp, "127.0.0.1", 8000),
    vds::for_each<network_socket>::create_handler(socket_session(this->router_, this->certificate_, this->private_key_))
  )
  (
    this->http_server_done_,
    this->http_server_error_
  );

  for (;;) {
    std::cout << "Enter command:\n";

    std::string cmd;
    std::cin >> cmd;

    if ("exit" == cmd) {
      break;
    }
  }
}

void vds::background_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
}

void vds::background_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->start_server_command_set_);
}

void vds::background_app::http_server_closed()
{
}

void vds::background_app::http_server_error(std::exception *)
{
}

vds::background_app::socket_session::socket_session(
  const vds::http_router & router,
  const certificate & certificate,
  const asymmetric_private_key & private_key)
  : router_(router), certificate_(certificate),
  private_key_(private_key)
{
}

vds::background_app::socket_session::handler::handler(
  const socket_session & owner,
  vds::network_socket & s)
: s_(std::move(s)),
  peer_(false, &owner.certificate_, &owner.private_key_),
  certificate_(owner.certificate_),
  private_key_(owner.private_key_),
  router_(owner.router_),
  done_handler_(this),
  error_handler_([this](std::exception *) {delete this; }),
  http_server_done_([this]() {}),
  http_server_error_([this](std::exception *) {})
{
}

void vds::background_app::socket_session::handler::start()
{
  std::cout << "New connection\n";

  vds::sequence(
    input_network_stream(this->s_),
    ssl_input_stream(this->peer_),
    http_parser(),
    http_middleware(this->router_),
    http_response_serializer(),
    ssl_output_stream(this->peer_),
    output_network_stream(this->s_)
  )
  (
    this->done_handler_,
    this->error_handler_
  );
}
