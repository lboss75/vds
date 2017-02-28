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
  client_(this->endpoints_)
{
}

void vds::background_app::main(const service_provider & sp)
{
  if(this->current_command_set_ == &this->start_server_command_set_){

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

void vds::background_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  registrator.add(this->task_manager_);
  
  if (&this->start_server_command_set_ == this->current_command_set_) {
    registrator.add(this->storage_);
    registrator.add(this->client_);
    registrator.add(this->server_);
  }
}

void vds::background_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->start_server_command_set_);
}


