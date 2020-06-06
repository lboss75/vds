/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"
#include "user_manager.h"
#include "keys_control.h"
#include "server_api.h"

vds::vds_cmd_app::vds_cmd_app()
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
  port_(
    "P",
    "port",
    "Port",
    "Port to listen connections"),
  dev_network_(
    "dev",
    "dev-network",
    "Development network",
    "Development network")
{
}

vds::expected<void> vds::vds_cmd_app::main(const service_provider * sp)
{
  if (this->current_command_set_ == &this->server_start_command_set_
    || &this->server_root_cmd_set_ == this->current_command_set_) {

    CHECK_EXPECTED(this->server_
      .start_network(
        (uint16_t)(this->port_.value().empty() ? 8050 : strtol(this->port_.value().c_str(), nullptr, 10)),
        this->dev_network_.value())
      .get());

    if (&this->server_root_cmd_set_ == this->current_command_set_) {
      GET_EXPECTED(data, file::read_all(filename("keys")));
      const_data_buffer root_private_key;
      binary_deserializer s(data);
      CHECK_EXPECTED(s >> root_private_key);

      keys_control::private_info_t private_info;

      GET_EXPECTED(root_private_key_key,
        asymmetric_private_key::parse_der(root_private_key, this->user_password_.value()));

      private_info.root_private_key_ = std::make_shared<asymmetric_private_key>(std::move(root_private_key_key));

        server_api api(sp);
        CHECK_EXPECTED(user_manager::reset(
        api,
        this->user_login_.value(),
        this->user_password_.value(),
        private_info).get());
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

  return expected<void>();
}

void vds::vds_cmd_app::register_services(vds::service_registrator& registrator)
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

void vds::vds_cmd_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->server_start_command_set_);
  this->server_start_command_set_.optional(this->port_);
  this->server_start_command_set_.optional(this->dev_network_);

  cmd_line.add_command_set(this->server_root_cmd_set_);
  this->server_root_cmd_set_.required(this->user_login_);
  this->server_root_cmd_set_.required(this->user_password_);
  this->server_root_cmd_set_.optional(this->port_);
  this->server_root_cmd_set_.optional(this->dev_network_);

  //cmd_line.add_command_set(this->server_init_command_set_);
  //this->server_init_command_set_.required(this->user_login_);
  //this->server_init_command_set_.required(this->user_password_);
  //this->server_init_command_set_.optional(this->node_name_);
  //this->server_init_command_set_.optional(this->port_);
}

vds::expected<void> vds::vds_cmd_app::start_services(service_registrator & registrator, service_provider * sp)
{
  if (&this->server_root_cmd_set_ == this->current_command_set_) {
    GET_EXPECTED(folder, persistence::current_user(sp));
    CHECK_EXPECTED(folder.delete_folder(true));
    CHECK_EXPECTED(folder.create());
    CHECK_EXPECTED(registrator.start());
  //} else if (&this->server_init_command_set_ == this->current_command_set_) {
  //  foldername folder(persistence::current_user(sp), ".vds");
  //  folder.delete_folder(true);
  //  folder.create();
  //  registrator.start(sp);
  }
  else {
    CHECK_EXPECTED(base_class::start_services(registrator, sp));
  }

  return expected<void>();
}

bool vds::vds_cmd_app::need_demonize()
{
  return false;
  //return (this->current_command_set_ == &this->server_start_command_set_);
}
