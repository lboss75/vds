/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "web_server_app.h"
#include "http_router.h"

vds::web_server_app::web_server_app()
: server_start_command_set_("Server start", "Start server", "start", "server"),
  server_service_command_set_("Service start", "Start service", "service", "server"),
  port_(
    "P",
    "port",
    "Port",
    "Port to listen connections"),
  dev_network_(
    "dev",
    "dev-network",
    "Use development network",
    "Use development network")
{
}

vds::expected<void> vds::web_server_app::main(const service_provider * sp)
{
  if (this->current_command_set_ == &this->server_start_command_set_
    || this->current_command_set_ == &this->server_service_command_set_) {
    std::shared_ptr<std::exception> error;

    const auto port = (uint16_t)(this->port_.value().empty() ? 8050 : strtol(this->port_.value().c_str(), nullptr, 10));
    CHECK_EXPECTED(this->server_.start_network(port, this->dev_network_.value()).get());
    
    std::cout << "Open http://localhost:" << port << "\n";

    if (this->current_command_set_ == &this->server_start_command_set_) {
      for (;;) {
        std::cout << "Enter command:\n";

        std::string cmd;
        std::cin >> cmd;

        if ("exit" == cmd) {
          break;
        }
      }
    }
    else {
      this->waiting_stop_signal();
    }
  }

  return expected<void>();
}

void vds::web_server_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->mt_service_);
  registrator.add(this->task_manager_);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  
  if (&this->server_start_command_set_ == this->current_command_set_
    || &this->server_service_command_set_ == this->current_command_set_){
    registrator.add(this->server_);
    if(!this->port_.value().empty()) {
      this->web_server_.port(atoi(this->port_.value().c_str()));
    }
    registrator.add(this->web_server_);
  }
}

void vds::web_server_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->server_start_command_set_);
  this->server_start_command_set_.optional(this->port_);
  this->server_start_command_set_.optional(this->dev_network_);

  cmd_line.add_command_set(this->server_service_command_set_);
  this->server_service_command_set_.optional(this->port_);
  this->server_service_command_set_.optional(this->dev_network_);
}

bool vds::web_server_app::need_demonize()
{
  return (this->current_command_set_ == &this->server_service_command_set_);
}
