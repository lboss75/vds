/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"

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
  device_actiovation_(
    "a",
    "device-activation",
    "Device activation file",
    "Specify file to activate device"),
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
  if(&this->server_root_cmd_set_ == this->current_command_set_){
    vds::imt_service::enable_async(sp);

    std::shared_ptr<std::exception> error;
    vds::barrier b;
    this->server_
        .reset(sp,
               this->user_login_.value(),
               this->user_password_.value())
        .execute([&error, &b, this](const std::shared_ptr<std::exception> & ex, const vds::device_activation & device_activation) {
          if (ex) {
            error = ex;
          }
          else {
            vds::file f(vds::filename(this->device_actiovation_.value()), vds::file::file_mode::truncate);
            f.write(device_activation.pack(this->user_password_.value()));
            f.close();
          }

          b.set();
        });

    b.wait();
    if(error){
      std::cout << "Failed:" << error->what() << "\n";
    }
  } else if(&this->server_init_command_set_ == this->current_command_set_){
    vds::imt_service::enable_async(sp);

    auto device_actiovation = vds::device_activation::unpack(
      vds::file::read_all(vds::filename(this->device_actiovation_.value())),
      this->user_password_.value());
    
    std::shared_ptr<std::exception> error;
    vds::barrier b;
    this->server_
        .init_server(
            sp,
            device_actiovation,
            this->user_password_.value(),
            this->node_name_.value(),
            this->port_.value().empty() ? 0 : atoi(this->port_.value().c_str()))
        .execute([&error, &b](const std::shared_ptr<std::exception> & ex) {
          if (ex) {
            error = ex;
          }

          b.set();
        });

    b.wait();
    if(error){
      std::cout << "Failed:" << error->what() << "\n";
    }
  } else if(this->current_command_set_ == &this->server_start_command_set_){
    std::shared_ptr<std::exception> error;
    vds::barrier b;
    this->server_
        .start_network(sp, (uint16_t)(this->port_.value().empty() ? 8050 : strtol(this->port_.value().c_str(), nullptr, 10)))
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
  this->server_root_cmd_set_.required(this->device_actiovation_);
  this->server_root_cmd_set_.optional(this->node_name_);
  this->server_root_cmd_set_.optional(this->port_);

  cmd_line.add_command_set(this->server_init_command_set_);
  this->server_init_command_set_.required(this->user_login_);
  this->server_init_command_set_.required(this->user_password_);
  this->server_init_command_set_.required(this->device_actiovation_);
  this->server_init_command_set_.optional(this->node_name_);
  this->server_init_command_set_.optional(this->port_);
}

void vds::background_app::start_services(service_registrator & registrator, service_provider & sp)
{
  if (&this->server_root_cmd_set_ == this->current_command_set_) {
    foldername folder(persistence::current_user(sp), ".vds");
    folder.delete_folder(true);
    folder.create();
    registrator.start(sp);
  } else if (&this->server_init_command_set_ == this->current_command_set_) {
    foldername folder(persistence::current_user(sp), ".vds");
    folder.delete_folder(true);
    folder.create();
    registrator.start(sp);
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
