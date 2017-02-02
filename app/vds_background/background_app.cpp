/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"

vds::background_app::background_app()
  :
  http_server_done_([this]() { this->http_server_closed(); }),
  http_server_error_([this](std::exception * ex) { this->http_server_error(ex); }),
  start_server_command_set_("Server start", "Start web server", "start", "server")
{
  this->router_.add_static(
    "/",
    "<html><body>Hello World</body></html>");
}

void vds::background_app::main(const service_provider & sp)
{
  vds::sequence(
    vds::socket_server(sp, "127.0.0.1", 8000),
    vds::for_each<vds::network_socket>::create_handler(socket_session(this->router_))
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

vds::background_app::socket_session::socket_session(const vds::http_router & router)
  : router_(router)
{
}

vds::background_app::socket_session::handler::handler(const socket_session & owner, vds::network_socket & s)
: s_(std::move(s)),
  router_(owner.router_),
  done_handler_(this),
  error_handler_([this](std::exception *) {delete this; })
{
}

void vds::background_app::socket_session::handler::start()
{
  std::cout << "New connection\n";

  vds::sequence(
    vds::input_network_stream(this->s_),
    vds::http_parser(),
    vds::http_middleware(this->router_),
    vds::http_response_serializer(),
    vds::output_network_stream(this->s_)
  )
  (
    this->done_handler_,
    this->error_handler_
    );
}
