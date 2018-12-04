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
#include "http_client.h"
#include "http_response.h"
#include "http_multipart_request.h"

namespace vds {
  class http_multipart_request;
}

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
  channel_id_(
    "c",
    "channel",
    "Channel ID",
    "Channel identifier to send message"),
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
    auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

    auto s = tcp_network_socket::connect(sp, network_address::parse(server));
    auto [reader, writer] = s->start(sp);

    auto client = std::make_shared<http_client>();
    auto f = client->start(reader, writer);

    bool operation_failed = true;
    std::string session;
    client->send(http_request::create(
      "GET",
      "/api/login?login=" + url_encode::encode(this->user_login_.value())
      + "&password=" + url_encode::encode(this->user_password_.value())).get_message(),
      [server, &session, &operation_failed](const http_message response) -> async_task<void>{

      http_response login_response(response);

      if (login_response.code() != http_response::HTTP_OK) {
        std::cerr << "Login failed\n";
        co_return;
      }

      auto body = json_parser::parse(
        server + "/api/login",
        co_await response.body()->read_all());

      auto body_object = dynamic_cast<const json_object *>(body.get());

      std::string value;
      body_object->get_property("state", value);

      if ("successful" != value) {
        co_return;
      }

      body_object->get_property("session", session);
      operation_failed = false;
    }).get();

    if(operation_failed) {
      std::cerr << "Login failed\n";
      return;
    }

    std::cout << "Login successful" << std::endl;

    http_multipart_request request(
      "POST",
      "/upload?session=" + url_encode::encode(session));

    request.add_string("channel_id", this->channel_id_.value());
    if (!this->message_.value().empty()) {
      request.add_string("message", this->message_.value());
    }

    filename fn(this->attachment_.value());
    request.add_file(fn, fn.name());

    operation_failed = true;
    client->send(request.get_message(),
      [server, &operation_failed](const http_message response) -> async_task<void> {

      http_response login_response(response);

      if (login_response.code() != http_response::HTTP_OK) {
        co_return;
      }

      operation_failed = false;
    }).get();

    if (operation_failed) {
      std::cerr << "Upload failed\n";
      return;
    }

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
  this->file_upload_cmd_set_.required(this->channel_id_);
  this->file_upload_cmd_set_.optional(this->message_);
  this->file_upload_cmd_set_.required(this->attachment_);
  this->file_upload_cmd_set_.optional(this->output_folder_);

  cmd_line.add_command_set(this->file_download_cmd_set_);
  this->file_download_cmd_set_.required(this->user_login_);
  this->file_download_cmd_set_.required(this->user_password_);
  this->file_download_cmd_set_.optional(this->server_);
  this->file_download_cmd_set_.required(this->channel_id_);
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
