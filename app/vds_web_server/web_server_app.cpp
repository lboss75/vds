/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "web_server_app.h"
#include "http_router.h"

vds::web_server_app::web_server_app()
: server_start_command_set_("Server start", "Start server", "start", "server"),
  start_web_(
    "w",
    "web",
    "Start Web",
    "Start web server"),
  port_(
    "P",
    "port",
    "Port",
    "Port to listen connections")
{
}

void vds::web_server_app::main(const service_provider & sp)
{
  if(this->current_command_set_ == &this->server_start_command_set_){
    std::shared_ptr<std::exception> error;
    vds::barrier b;
    this->server_
        .start_network(sp)
        .execute([&b, &error](const std::shared_ptr<std::exception> & ex){
      if(ex){
        error = ex;
      }
      b.set();
    });
    b.wait();
    if(error){
      std::cout << "Failed:" << error->what() << "\n";
    }

    for (;;) {
      std::cout << "Enter command:\n";

      std::string cmd;
      std::cin >> cmd;

      if ("exit" == cmd) {
        break;
      }
    }
  }
}

void vds::web_server_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->mt_service_);
  registrator.add(this->task_manager_);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  
  if (&this->server_start_command_set_ == this->current_command_set_){
    registrator.add(this->server_);
    if (!this->start_web_.value().empty()) {
      this->web_server_.static_root(this->start_web_.value());
      registrator.add(this->web_server_);
    }
  }
}

void vds::web_server_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->server_start_command_set_);
  this->server_start_command_set_.optional(this->start_web_);
  this->server_start_command_set_.optional(this->port_);
}

void vds::web_server_app::start_services(service_registrator & registrator, service_provider & sp)
{
  base_class::start_services(registrator, sp);
}

bool vds::web_server_app::need_demonize()
{
  return false;
  //return (this->current_command_set_ == &this->server_start_command_set_);
}