/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"
#include "user_manager.h"

vds::background_app::background_app()
: server_start_command_set_("Server start", "Start server", "start", "server"),
  server_root_cmd_set_("Install Root node", "Create new network", "root", "server"),
  server_init_command_set_("Initialize new node", "Attach this device to the network", "init", "server"),
  user_login_(
      "l",
      "login",
      "Login",
      "User login"),
  user_password_(
      "p",
      "password",
      "Password",
      "User password"),
  node_name_(
    "n",
    "name",
    "Node name",
    "Node name"),
  port_(
    "P",
    "port",
    "Port",
    "Port to listen connections")
{
}

void vds::background_app::main(const service_provider & sp)
{
  if (this->current_command_set_ == &this->server_start_command_set_
    || &this->server_root_cmd_set_ == this->current_command_set_) {

    this->server_
      .start_network(sp, (uint16_t)(this->port_.value().empty() ? 8050 : strtol(this->port_.value().c_str(), nullptr, 10)))
      .wait();

    if (&this->server_root_cmd_set_ == this->current_command_set_) {
      auto scope = sp.create_scope(__FUNCTION__);
      imt_service::enable_async(scope);

      auto user_mng = std::make_shared<user_manager>();
      user_mng->reset(scope,
        this->user_login_.value(),
        this->user_password_.value());
    }
    else {
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
}

void vds::background_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->mt_service_);
  registrator.add(this->task_manager_);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  
  if (&this->server_start_command_set_ == this->current_command_set_
      || &this->server_root_cmd_set_ == this->current_command_set_
      || &this->server_init_command_set_ == this->current_command_set_){
    registrator.add(this->server_);
  }
}

void vds::background_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->server_start_command_set_);

  cmd_line.add_command_set(this->server_root_cmd_set_);
  this->server_root_cmd_set_.required(this->user_login_);
  this->server_root_cmd_set_.required(this->user_password_);
  this->server_root_cmd_set_.optional(this->node_name_);
  this->server_root_cmd_set_.optional(this->port_);

  //cmd_line.add_command_set(this->server_init_command_set_);
  //this->server_init_command_set_.required(this->user_login_);
  //this->server_init_command_set_.required(this->user_password_);
  //this->server_init_command_set_.optional(this->node_name_);
  //this->server_init_command_set_.optional(this->port_);
}

void vds::background_app::start_services(service_registrator & registrator, service_provider & sp)
{
  if (&this->server_root_cmd_set_ == this->current_command_set_) {
    foldername folder(persistence::current_user(sp), ".vds");
    folder.delete_folder(true);
    folder.create();
    registrator.start(sp);
  //} else if (&this->server_init_command_set_ == this->current_command_set_) {
  //  foldername folder(persistence::current_user(sp), ".vds");
  //  folder.delete_folder(true);
  //  folder.create();
  //  registrator.start(sp);
  }
  else {
    base_class::start_services(registrator, sp);
  }
}

bool vds::background_app::need_demonize()
{
  return false;
  //return (this->current_command_set_ == &this->server_start_command_set_);
}
