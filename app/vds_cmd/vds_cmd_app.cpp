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
  file_sync_cmd_set_("Synchronize files", "Synchronize local files with the network", "sync", "file"),
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
  sync_style_(
    "s",
    "sync-style",
    "Sync style",
    "Synchronize style: default - set local state equal to channel state, "
    "download - download new files from the network, "
    "upload - upload files to the network and remove its local copy"
  ),
  exclude_(
    "e",
    "exclude",
    "Exclude filter",
    "Exclude filter: folder/ or *.ext separated by ;"
  ),
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

vds::expected<void> vds::vds_cmd_app::main(const service_provider * sp)
{
  if (this->current_command_set_ == &this->file_upload_cmd_set_) {
    GET_EXPECTED(session, this->login(sp));
    CHECK_EXPECTED(this->upload_file(sp, session));
    CHECK_EXPECTED(this->logout(sp, session));
  }
  else if (this->current_command_set_ == &this->file_sync_cmd_set_) {
    GET_EXPECTED(session, this->login(sp));
    CHECK_EXPECTED(this->sync_files(sp, session));
    CHECK_EXPECTED(this->logout(sp, session));
  }
  else if (this->current_command_set_ == &this->channel_list_cmd_set_) {
    GET_EXPECTED(session, this->login(sp));
    CHECK_EXPECTED(this->channel_list(sp, session));
    CHECK_EXPECTED(this->logout(sp, session));
  }
  else if (this->current_command_set_ == &this->channel_create_cmd_set_) {
    GET_EXPECTED(session, this->login(sp));
    CHECK_EXPECTED(this->channel_create(sp, session));
    CHECK_EXPECTED(this->logout(sp, session));
  }

  return expected<void>();
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

  cmd_line.add_command_set(this->file_sync_cmd_set_);
  this->file_sync_cmd_set_.required(this->user_login_);
  this->file_sync_cmd_set_.required(this->user_password_);
  this->file_sync_cmd_set_.optional(this->server_);
  this->file_sync_cmd_set_.required(this->channel_id_);
  this->file_sync_cmd_set_.optional(this->sync_style_);
  this->file_sync_cmd_set_.required(this->output_folder_);
  this->file_sync_cmd_set_.optional(this->exclude_);

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


bool vds::vds_cmd_app::need_demonize()
{
  return false;
}

vds::expected<void> vds::vds_cmd_app::invoke_server(
  const service_provider * sp,
  vds::http_message request,
  const std::function<vds::async_task<vds::expected<void>>(vds::http_message response)> & response_handler)
{
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  GET_EXPECTED(address, network_address::parse(server));
  GET_EXPECTED(s, tcp_network_socket::connect(sp, address));
  GET_EXPECTED(streams, s->start(sp));
  auto reader = std::get<0>(streams);
  auto writer = std::get<1>(streams);

  auto client = std::make_shared<http_client>();
  auto client_task = client->start(reader, writer);

  CHECK_EXPECTED(client->send(request, response_handler).get());

  CHECK_EXPECTED(s->close());
  CHECK_EXPECTED(client_task.get());

  return expected<void>();
}

vds::expected<std::string> vds::vds_cmd_app::login(const service_provider * sp)
{
  std::string session;
  CHECK_EXPECTED(this->invoke_server(
    sp,
    http_request::create(
      "GET",
      "/api/login?login=" + url_encode::encode(this->user_login_.value())
      + "&password=" + url_encode::encode(this->user_password_.value())).get_message(),
    [&session](const http_message response) -> async_task<expected<void>> {

    http_response login_response(response);

    if (login_response.code() != http_response::HTTP_OK) {
      co_return vds::make_unexpected<std::runtime_error>("Login failed");
    }

    const_data_buffer response_body;
    GET_EXPECTED_VALUE_ASYNC(response_body, co_await response.body()->read_all());
    GET_EXPECTED_ASYNC(body, json_parser::parse("/api/login", response_body));

    const auto body_object = dynamic_cast<const json_object *>(body.get());

    std::string value;
    CHECK_EXPECTED_ASYNC(body_object->get_property("state", value));

    if ("successful" != value) {
      co_return vds::make_unexpected<std::runtime_error>("Login failed " + value);
    }

    CHECK_EXPECTED_ASYNC(body_object->get_property("session", session));
    co_return expected<void>();
  }));

  return session;
}

vds::expected<void> vds::vds_cmd_app::logout(const service_provider* sp, const std::string& session) {
  return this->invoke_server(
    sp,
    http_request::create(
      "POST",
      "/api/logout?session=" + url_encode::encode(session)).get_message(),
    [](const http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK && http_response(response).code() != http_response::HTTP_Found) {
      co_return vds::make_unexpected<std::runtime_error>("Logout failed " + http_response(response).comment());
    }

    co_return expected<void>();
  });
}


vds::expected<void> vds::vds_cmd_app::upload_file(const service_provider * sp, const std::string & session) {
  filename fn(this->attachment_.value());
  return this->upload_file(sp, session, fn, fn.name());
}

vds::expected<void> vds::vds_cmd_app::upload_file(const service_provider* sp, const std::string& session,
  const filename& fn, const std::string& name) {

  http_multipart_request request(
    "POST",
    "/api/upload?session=" + url_encode::encode(session));

  request.add_string("channel_id", this->channel_id_.value());
  if (!this->message_.value().empty()) {
    request.add_string("message", this->message_.value());
  }


  auto mimetype = http_mimetype::mimetype(fn);
  if (mimetype.empty()) {
    mimetype = "application/octet-stream";
  }

  CHECK_EXPECTED(request.add_file("attachment", fn, name, mimetype));

  return this->invoke_server(
    sp,
    request.get_message(),
    [](const http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      co_return vds::make_unexpected<std::runtime_error>("Upload failed " + http_response(response).comment());
    }

    co_return expected<void>();
  });
}

vds::expected<void> vds::vds_cmd_app::download_file(
  const service_provider* sp,
  const std::string& session) {
  return make_unexpected<vds_exceptions::invalid_operation>();
}

vds::expected<void> vds::vds_cmd_app::download_file(
  const service_provider* sp,
  const std::string& session,
  const filename& fn,
  const const_data_buffer& file_id) {
  return make_unexpected<vds_exceptions::invalid_operation>();
}


vds::expected<void> vds::vds_cmd_app::sync_files(const service_provider* sp, const std::string& session) {
  return this->invoke_server(
    sp,
    http_request::create(
      "GET",
      "/api/channel_feed?session=" + url_encode::encode(session)
      + "&channel_id=" + url_encode::encode(this->channel_id_.value())).get_message(),
    [this, sp, session](const http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Query channel failed " + http_response(response).comment());
    }

    GET_EXPECTED(response_body, response.body()->read_all().get());

    GET_EXPECTED(body, json_parser::parse(
      "/api/channel_feed",
      response_body));

    //Collect file state
    std::map<std::string, std::list<sync_file_info>> name2file;
    std::map<const_data_buffer, std::list<sync_file_info>> hash2file;
    const auto messages = dynamic_cast<const json_array *>(body.get());
    if (nullptr != messages) {
      for (size_t i = 0; i < messages->size(); ++i) {
        const auto item = dynamic_cast<const json_object *>(messages->get(messages->size() - i - 1).get());//Reverse
        if (nullptr != item) {
          std::shared_ptr<json_value> files_prop = item->get_property("files");
          if (!files_prop) {
            continue;
          }

          const auto files = dynamic_cast<const json_array *>(files_prop.get());
          if (nullptr == files) {
            continue;
          }

          for (size_t n = 0; n < files->size(); ++n) {
            const auto f = dynamic_cast<const json_object *>(files->get(n).get());

            const_data_buffer object_id;
            auto ret = f->get_property("object_id", object_id);
            if (ret.has_error() || 0 == object_id.size()) {
              continue;
            }

            std::string file_name;
            ret = f->get_property("name", file_name);
            if (ret.has_error()) {
              continue;
            }

            std::string mimetype;
            ret = f->get_property("mimetype", mimetype);
            if (ret.has_error()) {
              continue;
            }

            size_t size;
            ret = f->get_property("size", size);
            if (ret.has_error()) {
              continue;
            }

            sync_file_info info{
              object_id,
              file_name,
              mimetype,
              size
            };

            name2file[file_name].push_back(info);
            hash2file[object_id].push_back(info);
          }
        }
      }
    }


    //Scan folder
    auto filters = split_string(this->exclude_.value(), ';', true);

    foldername sync_folder(this->output_folder_.value());

    std::map<filename, bool> files;
    (void)sync_folder.files([&filters, &files](const filename & fn) -> expected<bool> {
      bool filtered = false;
      for (const auto & filter : filters) {
        if (filter.empty()) {
          continue;
        }

        if ('/' == filter[filter.size() - 1]) {
          //It folder
          if (fn.full_name().size() > filter.size() && fn.full_name().substr(0, filter.size()) == filter) {
            filtered = true;
            break;
          }
        }
        else if ('*' == filter[0]) {
          //file extension
          if (fn.extension() == filter) {
            filtered = true;
            break;
          }
        }
        else {
          //file name
          if (fn.name() == filter) {
            filtered = true;
            break;
          }
        }
      }

      if (!filtered) {
        files[fn] = false;
      }

      return true;
    });

    if (this->sync_style_.value().empty()
      || "default" == this->sync_style_.value()
      || "upload" == this->sync_style_.value()) {
      for (const auto & fn : files) {
        auto name = fn.first.full_name().substr(0, sync_folder.full_name().size() + 1);
        auto p = name2file.find(name);
        if (name2file.end() == p) {
          //Upload
          CHECK_EXPECTED(this->upload_file(sp, session, fn.first, name));
        }
        else {
          CHECK_EXPECTED(this->sync_file(sp, session, fn.first, name, p->second));
        }
      }
    }

    if (this->sync_style_.value().empty()
      || "default" == this->sync_style_.value()
      || "download" == this->sync_style_.value()) {
      for (const auto & name : name2file) {
        filename fn(sync_folder, name.first);
        auto p = files.find(fn);
        if (files.end() == p) {
          //Upload
          CHECK_EXPECTED(this->download_file(sp, session, fn, name.second.front().object_id_));
        }
        else {
          //CHECK_EXPECTED(this->sync_file(sp, session, fn, name.first, p->second));

        }
      }
    }

    return expected<void>();
  });
}

vds::expected<void> vds::vds_cmd_app::channel_list(const service_provider* sp, const std::string& session) {
  return this->invoke_server(
    sp,
    http_request::create(
      "GET",
      "/api/channels?session=" + url_encode::encode(session)).get_message(),
    [this](const http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Query channels failed " + http_response(response).comment());
    }

    return this->channel_list_out(response);
  });
}

vds::expected<void> vds::vds_cmd_app::channel_create(const service_provider* sp, const std::string& session) {
  return this->invoke_server(
    sp,
    http_request::create(
      "POST",
      "/api/channels?session=" + url_encode::encode(session)
      + "&name=" + url_encode::encode(this->channel_name_.value())
      + "&type=" + url_encode::encode(this->channel_type_.value())
    ).get_message(),
    [this](const http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      co_return vds::make_unexpected<std::runtime_error>("Create channel failed");
    }

    co_return expected<void>();
  });
}

vds::async_task<vds::expected<void>> vds::vds_cmd_app::channel_list_out(const http_message response) {

  const_data_buffer response_body;
  GET_EXPECTED_VALUE_ASYNC(response_body, co_await response.body()->read_all());

  if (this->output_format_.value() == "json") {
    std::cout << std::string((const char *)response_body.data(), response_body.size()) << std::endl;
  }
  else {
    GET_EXPECTED_ASYNC(body, json_parser::parse(
      "/api/channels",
      response_body));

    std::cout << std::setw(44) << std::left << "ID" << "|"
      << std::setw(15) << std::left << "Type" << "|"
      << "Name" << std::endl;

    auto body_array = dynamic_cast<const json_array *>(body.get());
    for (size_t i = 0; i < body_array->size(); ++i) {
      auto item = dynamic_cast<const json_object *>(body_array->get(i).get());

      std::string value;
      CHECK_EXPECTED_ASYNC(item->get_property("object_id", value));
      std::cout << std::setw(44) << value << "|";

      CHECK_EXPECTED_ASYNC(item->get_property("type", value));
      std::cout << std::setw(15) << std::left << value << "|";

      CHECK_EXPECTED_ASYNC(item->get_property("name", value));
      std::cout << value << std::endl;
    }
  }
  co_return expected<void>();
}

vds::expected<void> vds::vds_cmd_app::sync_file(
  const service_provider* sp,
  const std::string& session,
  const filename& exists_files,
  const std::string& rel_name,
  const std::list<sync_file_info>& file_history) {

  return expected<void>();

}
