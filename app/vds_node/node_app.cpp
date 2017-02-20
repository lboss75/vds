/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node_app.h"

vds::node_app::node_app()
:
add_storage_cmd_set_(
  "Add new storage",
  "Add new storage to the network",
  "add",
  "storage"
),
remove_storage_cmd_set_(
  "Remove storage",
  "Remove storage to the network",
  "remove",
  "storage"
),
list_storage_cmd_set_(
  "List storage",
  "List storage to the network",
  "list",
  "storage"
),
storage_path_(
  "s", "storage",
  "Storage", "Path to the storage"
),

node_install_cmd_set_(
  "Install Node",
  "Install new node",
  "install",
  "node"
),
node_login_(
  "l",
  "login",
  "Login",
  "User login"
),
node_password_(
  "p",
  "password",
  "Password",
  "User password"
)
{

}

void vds::node_app::main(
  const vds::service_provider& sp)
{
  if (&this->node_install_cmd_set_ == this->current_command_set_) {
    std::cout << "Waiting for network connection\n";

    this->client_.node_install(
      this->node_login_.value(),
      this->node_password_.value());

    sp.get<vsr_protocol::iclient>().subscribe_client_id_assigned([this](vsr_protocol::client & client){
      this->cliend_id_assigned_.set();
    });
    
    if(!this->cliend_id_assigned_.wait_for(std::chrono::seconds(15))){
      throw new std::runtime_error("Connection failed");
    }
  }
}

void vds::node_app::register_command_line(vds::command_line& cmd_line)
{
  cmd_line.add_command_set(this->add_storage_cmd_set_);
  this->add_storage_cmd_set_.required(this->storage_path_);

  cmd_line.add_command_set(this->node_install_cmd_set_);
  this->node_install_cmd_set_.required(this->node_login_);
  this->node_install_cmd_set_.required(this->node_password_);
}

void vds::node_app::register_services(service_registrator & registrator)
{
  base_class::register_services(registrator);
  
  if (&this->node_install_cmd_set_ == this->current_command_set_) {
    registrator.add(this->task_manager_);
    registrator.add(this->storage_service_);
    registrator.add(this->network_service_);
    registrator.add(this->crypto_service_);
    registrator.add(this->client_);
  }
}
