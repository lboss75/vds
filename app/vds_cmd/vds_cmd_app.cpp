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
#include "http_mimetype.h"
#include <iomanip>

namespace vds {
  class http_multipart_request;
}

vds::vds_cmd_app::vds_cmd_app()
: file_upload_cmd_set_("Upload file", "Upload file on the network", "upload", "file"),
  file_download_cmd_set_("Download file", "Download file from the network", "download", "file"),
  channel_list_cmd_set_("Channel list", "List user channels", "list", "channel"),
  channel_create_cmd_set_("Channel create", "Create new channel", "create", "channel"),
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
    "a",
    "attachment",
    "File(s) names",
    "Comma separated file names"),
  output_folder_(
    "o",
    "output",
    "output folder",
    "Folder to store files"),
  output_format_(
    "f",
    "format",
    "Output format",
    "Output format (json)"
  ),
  channel_name_(
    "cn",
    "channel-name",
    "Channel name",
    "Name of channel"),
  channel_type_(
    "ct",
    "channel-type",
    "Channel type",
    "Type of channel") {
}

void vds::vds_cmd_app::main(const service_provider * sp)
{
  if (this->current_command_set_ == &this->file_upload_cmd_set_) {
    const auto session = this->login(sp);
    this->upload_file(sp, session);
  }
  else if(this->current_command_set_ == &this->channel_list_cmd_set_) {
    const auto session = this->login(sp);
    this->channel_list(sp, session);
  }
  else if (this->current_command_set_ == &this->channel_create_cmd_set_) {
    const auto session = this->login(sp);
    this->channel_create(sp, session);
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

  cmd_line.add_command_set(this->channel_list_cmd_set_);
  this->channel_list_cmd_set_.required(this->user_login_);
  this->channel_list_cmd_set_.required(this->user_password_);
  this->channel_list_cmd_set_.optional(this->server_);
  this->channel_list_cmd_set_.optional(this->output_format_);

  cmd_line.add_command_set(this->channel_create_cmd_set_);
  this->channel_create_cmd_set_.required(this->user_login_);
  this->channel_create_cmd_set_.required(this->user_password_);
  this->channel_create_cmd_set_.optional(this->server_);
  this->channel_create_cmd_set_.optional(this->channel_name_);
  this->channel_create_cmd_set_.optional(this->channel_type_);

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

std::string vds::vds_cmd_app::login(const service_provider * sp)
{
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  auto s = tcp_network_socket::connect(sp, network_address::parse(server));
  auto[reader, writer] = s->start(sp);

  auto client = std::make_shared<http_client>();
  client->start(reader, writer).detach();

  std::string session;
  client->send(http_request::create(
    "GET",
    "/api/login?login=" + url_encode::encode(this->user_login_.value())
    + "&password=" + url_encode::encode(this->user_password_.value())).get_message(),
    [server, &session](const http_message response) -> async_task<void> {

    http_response login_response(response);

    if (login_response.code() != http_response::HTTP_OK) {
      throw std::runtime_error("Login failed");
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
  }).get();

  return session;
}

void vds::vds_cmd_app::upload_file(const service_provider * sp, const std::string & session) {
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  auto s = tcp_network_socket::connect(sp, network_address::parse(server));
  auto[reader, writer] = s->start(sp);

  auto client = std::make_shared<http_client>();
  client->start(reader, writer).detach();

  http_multipart_request request(
    "POST",
    "/api/upload?session=" + url_encode::encode(session));

  request.add_string("channel_id", this->channel_id_.value());
  if (!this->message_.value().empty()) {
    request.add_string("message", this->message_.value());
  }

  filename fn(this->attachment_.value());

  auto mimetype = http_mimetype::mimetype(fn);
  if(mimetype.empty()) {
    mimetype = "application/octet-stream";
  }
  request.add_file("attachment", fn, fn.name(), mimetype);

  client->send(request.get_message(),
    [server](const http_message response) -> async_task<void> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      throw std::runtime_error("Upload failed");
    }

    co_return;
  }).get();
}

void vds::vds_cmd_app::channel_list(const service_provider* sp, const std::string& session) {
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  auto s = tcp_network_socket::connect(sp, network_address::parse(server));
  auto[reader, writer] = s->start(sp);

  auto client = std::make_shared<http_client>();
  client->start(reader, writer).detach();

  client->send(http_request::create(
    "GET",
    "/api/channels?session=" + url_encode::encode(session)).get_message(),
    [server, this](const http_message response) -> async_task<void> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      throw std::runtime_error("Query channels failed");
    }

    if (this->output_format_.value() == "json") {
      std::cout << co_await response.body()->read_all() << std::endl;
    }
    else {
      auto body = json_parser::parse(
        server + "/api/channels",
        co_await response.body()->read_all());

      std::cout << std::setw(44) << std::left << "ID" << "|"
      << std::setw(15) << std::left << "Type" << "|"
      << "Name" << std::endl;

      auto body_array = dynamic_cast<const json_array *>(body.get());
      for(size_t i = 0; i < body_array->size(); ++i) {
        auto item = dynamic_cast<const json_object *>(body_array->get(i).get());

        std::string value;
        item->get_property("object_id", value);
        std::cout << std::setw(44) << value << "|";

        item->get_property("type", value);
        std::cout << std::setw(15) << std::left << value << "|";

        item->get_property("name", value);
        std::cout << value << std::endl;
      }
    }
  }).get();
}

void vds::vds_cmd_app::channel_create(const service_provider* sp, const std::string& session) {
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  auto s = tcp_network_socket::connect(sp, network_address::parse(server));
  auto[reader, writer] = s->start(sp);

  auto client = std::make_shared<http_client>();
  client->start(reader, writer).detach();

  client->send(http_request::create(
    "POST",
    "/api/channels?session=" + url_encode::encode(session)
    + "&name=" + url_encode::encode(this->channel_name_.value())
    + "&type=" + url_encode::encode(this->channel_type_.value())
    ).get_message(),
    [server, this](const http_message response) -> async_task<void> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      throw std::runtime_error("Create channel failed");
    }

    co_return;
  }).get();
}
