/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_cmd_app.h"
#include "http_router.h"
#include "user_manager.h"
#include "tcp_network_socket.h"
#include "http_request.h"
#include "http_serializer.h"

vds::vds_cmd_app::vds_cmd_app()
: file_upload_cmd_set_("Upload file", "Upload file on the network", "upload", "file"),
  file_download_cmd_set_("Download file", "Download file from the network", "download", "file"),
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
  server_(
    "s",
    "server",
    "Server URL",
    "Server URL to connect"),
  message_(
    "m",
    "message",
    "Message",
    "Message"),
  attachment_(
    "f",
    "file",
    "File(s) names",
    "Comma separated file names"),
  output_folder_(
    "o",
    "output",
    "output folder",
    "Folder to store files") {
}

void vds::vds_cmd_app::main(const service_provider * sp)
{
  if (this->current_command_set_ == &this->file_upload_cmd_set_) {
    auto server = this->server_.value().empty() ? "localhost:8050" : this->server_.value();

    auto s = tcp_network_socket::connect(sp, network_address::parse(server));
    auto [reader, writer] = s->start(sp);
    auto http_out = std::make_shared<http_async_serializer>(writer);

    auto login_request = http_request::create(
      "GET",
      "api/login?login=" + url_encode::encode(this->user_login_.value())
      + "&password=" + url_encode::encode(this->user_password_.value));

    http_out->write_async(login_request.get_message()).get();

    auto client = http_parser();
  }
}

void vds::vds_cmd_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->mt_service_);
  registrator.add(this->task_manager_);
  registrator.add(this->network_service_);
}

void vds::vds_cmd_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->file_upload_cmd_set_);
  this->file_upload_cmd_set_.required(this->user_login_);
  this->file_upload_cmd_set_.required(this->user_password_);
  this->file_upload_cmd_set_.optional(this->server_);
  this->file_upload_cmd_set_.optional(this->message_);
  this->file_upload_cmd_set_.required(this->attachment_);
  this->file_upload_cmd_set_.optional(this->output_folder_);

  cmd_line.add_command_set(this->file_download_cmd_set_);
  this->file_download_cmd_set_.required(this->user_login_);
  this->file_download_cmd_set_.required(this->user_password_);
  this->file_download_cmd_set_.optional(this->server_);
  this->file_download_cmd_set_.optional(this->message_);
  this->file_download_cmd_set_.required(this->attachment_);
  this->file_download_cmd_set_.optional(this->output_folder_);

  //cmd_line.add_command_set(this->server_init_command_set_);
  //this->server_init_command_set_.required(this->user_login_);
  //this->server_init_command_set_.required(this->user_password_);
  //this->server_init_command_set_.optional(this->node_name_);
  //this->server_init_command_set_.optional(this->port_);
}

void vds::vds_cmd_app::start_services(service_registrator & registrator, service_provider * sp)
{
  base_class::start_services(registrator, sp);
}

bool vds::vds_cmd_app::need_demonize()
{
  return false;
}
