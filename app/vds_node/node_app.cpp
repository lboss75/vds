/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node_app.h"

vds::node_app::node_app()
:
  file_upload_cmd_set_(
    "Upload file to the network",
    "Upload file to the network",
    "upload",
    "file"
  ),
  file_download_cmd_set_(
    "Download file from the network",
    "Download file from the network",
    "download",
    "file"
  ),
  root_folder_(
    "r", "root",
    "Root folder", "Root folder to store files"
  ),
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
  login_(
    "l",
    "login",
    "Login",
    "User login"),
  password_(
    "p",
    "password",
    "Password",
    "User password"),
  filename_(
    "f",
    "file",
    "File path",
    "Full path to the file"
  ),
  client_("https://127.0.0.1:8050")
{
}

void vds::node_app::main(
  const vds::service_provider& sp)
{
  if (&this->node_install_cmd_set_ == this->current_command_set_) {
    std::cout << "Waiting for network connection\n";

    //this->client_.node_install(
    //  this->node_login_.value(),
    //  this->node_password_.value());

    //sp.get<vsr_protocol::iclient>().subscribe_client_id_assigned([this](vsr_protocol::client & client) {
    //  this->cliend_id_assigned_.set();
    //});

    //if (!this->cliend_id_assigned_.wait_for(std::chrono::seconds(15))) {
    //  throw std::runtime_error("Connection failed");
    //}
  }
  else if (&this->file_upload_cmd_set_ == this->current_command_set_) {
    filename fn(this->filename_.value());

    barrier b;
    sp.get<vds::iclient>()->upload_file(sp, this->login_.value(), this->password_.value(), fn.name(), fn)
      .wait(
        [&b](const vds::service_provider&sp, const std::string& version_id) {
          std::cout << "File uploaded " << version_id << "\n";
          b.set();
        },
        [](const vds::service_provider&sp, const std::shared_ptr<std::exception> & ex) {
          sp.unhandled_exception(ex);
        },
      sp);

    b.wait();
  }
  else if (&this->file_download_cmd_set_ == this->current_command_set_) {
    filename fn(this->filename_.value());

    barrier b;
    sp.get<vds::iclient>()->download_data(sp, this->login_.value(), this->password_.value(), fn.name(), fn)
      .wait(
        [&b](const vds::service_provider&sp, const guid& version_id) {
          std::cout << "File downloaded " << version_id.str() << "\n";
          b.set();
        },
        [](const vds::service_provider&sp, const std::shared_ptr<std::exception> & ex) {
          sp.unhandled_exception(ex);
        },
      sp);

    b.wait();
  }
}
/*
void vds::node_app::node_install(storage_log& log, const certificate& user, const asymmetric_private_key& user_key)
{
    std::cout << "Generating node private key\n";
  asymmetric_private_key key(asymmetric_crypto::rsa4096());
  key.generate();

  asymmetric_public_key pkey(key);

  std::cout << "Creating node certificate \n";
  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "Node Certificate";
  options.ca_certificate = &user;
  options.ca_certificate_private_key = &user_key;

  certificate node_cert = certificate::create_new(pkey, key, options);
  
  
  
}
*/

void vds::node_app::register_command_line(vds::command_line& cmd_line)
{
  cmd_line.add_command_set(this->add_storage_cmd_set_);
  this->add_storage_cmd_set_.required(this->storage_path_);

  cmd_line.add_command_set(this->node_install_cmd_set_);
  this->node_install_cmd_set_.required(this->login_);
  this->node_install_cmd_set_.required(this->password_);

  cmd_line.add_command_set(this->file_upload_cmd_set_);
  this->file_upload_cmd_set_.required(this->login_);
  this->file_upload_cmd_set_.required(this->password_);
  this->file_upload_cmd_set_.required(this->filename_);
  this->file_upload_cmd_set_.optional(this->root_folder_);

  cmd_line.add_command_set(this->file_download_cmd_set_);
  this->file_download_cmd_set_.required(this->login_);
  this->file_download_cmd_set_.required(this->password_);
  this->file_download_cmd_set_.required(this->filename_);
  this->file_download_cmd_set_.optional(this->root_folder_);
}

void vds::node_app::register_services(service_registrator & registrator)
{
  base_class::register_services(registrator);

  if (&this->node_install_cmd_set_ == this->current_command_set_) {
    registrator.add(this->task_manager_);
    registrator.add(this->network_service_);
    registrator.add(this->crypto_service_);
    registrator.add(this->client_);
  }
  else if (
     &this->file_upload_cmd_set_ == this->current_command_set_
  || &this->file_download_cmd_set_ == this->current_command_set_) {
    registrator.add(this->mt_service_);
    registrator.add(this->task_manager_);
    registrator.add(this->network_service_);
    registrator.add(this->crypto_service_);
    registrator.add(this->client_);
  }
}

void vds::node_app::start_services(service_registrator & registrator, service_provider & sp)
{
  if (!this->root_folder_.value().empty()) {
    vds::foldername folder(this->root_folder_.value());
    folder.create();

    auto root_folders = new vds::persistence_values();
    root_folders->current_user_ = folder;
    root_folders->local_machine_ = folder;
    sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);
  }

  base_class::start_services(registrator, sp);
}
