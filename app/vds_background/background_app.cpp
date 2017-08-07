/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"
#include "certificate_authority_p.h"

vds::background_app::background_app()
: server_start_command_set_("Server start", "Start web server", "start", "server"),
  root_folder_(
    "r", "root",
    "Root folder", "Root folder to store files"
  ),
  node_login_(
    "l",
    "login",
    "Login",
    "User login"),
  node_password_(
    "p",
    "password",
    "Password",
    "User password"),
  port_(
    "P",
    "port",
    "Port",
    "Port to listen connections"),
  server_root_cmd_set_(
    "Install Root node",
    "Create new network",
    "root",
    "server"),
  start_(
    "s", "start",
    "Start server",
    "Satrt server after creation")
{
}

void vds::background_app::main(const service_provider & sp)
{
  if(this->current_command_set_ == &this->server_start_command_set_
    || (&this->server_root_cmd_set_ == this->current_command_set_ && this->start_.value())){

    for (;;) {
      this->connection_manager_.start_servers(sp, "udp://127.0.0.1:" + (this->port_.value().empty() ? "8050" : this->port_.value()));

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
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  registrator.add(this->task_manager_);
  
  if (&this->server_start_command_set_ == this->current_command_set_
    || &this->server_root_cmd_set_ == this->current_command_set_) {
    registrator.add(this->server_);
    registrator.add(this->connection_manager_);
    registrator.add(this->server_log_sync_);
  }
}

void vds::background_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->server_start_command_set_);
  this->server_start_command_set_.optional(this->root_folder_);
  this->server_start_command_set_.optional(this->port_);

  cmd_line.add_command_set(this->server_root_cmd_set_);
  this->server_root_cmd_set_.required(this->node_password_);
  this->server_root_cmd_set_.optional(this->root_folder_);
  this->server_root_cmd_set_.optional(this->port_);
  this->server_root_cmd_set_.optional(this->start_);
}

void vds::background_app::start_services(service_registrator & registrator, service_provider & sp)
{
  if (!this->root_folder_.value().empty()) {
    vds::foldername folder(this->root_folder_.value());
    folder.create();

    auto root_folders = new vds::persistence_values();
    root_folders->current_user_ = folder;
    root_folders->local_machine_ = folder;
    sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  }

  if (&this->server_root_cmd_set_ == this->current_command_set_) {
    vds::asymmetric_private_key private_key(vds::asymmetric_crypto::rsa4096());
    private_key.generate();

    auto root_id = vds::guid::new_guid();
    vds::certificate root_certificate = vds::_certificate_authority::create_root_user(root_id, private_key);

    vds::asymmetric_private_key server_private_key(vds::asymmetric_crypto::rsa4096());
    server_private_key.generate();

    vds::guid current_server_id = vds::guid::new_guid();
    vds::certificate server_certificate = vds::certificate_authority::create_server(
      current_server_id,
      root_certificate,
      private_key,
      server_private_key);

    foldername folder(persistence::current_user(sp), ".vds");
    folder.create();

    server_certificate.save(vds::filename(folder, "server.crt"));
    server_private_key.save(vds::filename(folder, "server.pkey"));

    if (!this->port_.value().empty()) {
      this->server_.set_port(std::atoi(this->port_.value().c_str()));
    } else {
      this->server_.set_port(8050);
    }

    registrator.start(sp);

    sp.get<vds::istorage_log>()->reset(
      sp,
      root_id,
      root_certificate,
      private_key,
      this->node_password_.value(),
      "https://127.0.0.1:" + (this->port_.value().empty() ? "8050" : this->port_.value()));
  }
  else {
    if (&this->server_start_command_set_ == this->current_command_set_) {
      if (!this->port_.value().empty()) {
        this->server_.set_port(std::atoi(this->port_.value().c_str()));
      }
      else {
        this->server_.set_port(8050);
      }
    }

    base_class::start_services(registrator, sp);
  }
}

