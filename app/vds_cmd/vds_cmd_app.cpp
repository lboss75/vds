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
#include "stream.h"
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
    "ss",
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
    GET_EXPECTED(client, this->connect(sp));
    GET_EXPECTED(session, this->login(sp, client));
    CHECK_EXPECTED(this->upload_file(sp, client, session));
    CHECK_EXPECTED(this->logout(sp, client, session));
    CHECK_EXPECTED(client->close().get());
  }
  else if (this->current_command_set_ == &this->file_download_cmd_set_) {
    GET_EXPECTED(client, this->connect(sp));
    GET_EXPECTED(session, this->login(sp, client));
    CHECK_EXPECTED(this->download_file(sp, client, session));
    CHECK_EXPECTED(this->logout(sp, client, session));
    CHECK_EXPECTED(client->close().get());
  }
  else if (this->current_command_set_ == &this->file_sync_cmd_set_) {
    GET_EXPECTED(client, this->connect(sp));
    GET_EXPECTED(session, this->login(sp, client));
    CHECK_EXPECTED(this->sync_files(sp, client, session));
    CHECK_EXPECTED(this->logout(sp, client, session));
    CHECK_EXPECTED(client->close().get());
  }
  else if (this->current_command_set_ == &this->channel_list_cmd_set_) {
    GET_EXPECTED(client, this->connect(sp));
    GET_EXPECTED(session, this->login(sp, client));
    CHECK_EXPECTED(this->channel_list(sp, client, session));
    CHECK_EXPECTED(this->logout(sp, client, session));
    CHECK_EXPECTED(client->close().get());
  }
  else if (this->current_command_set_ == &this->channel_create_cmd_set_) {
    GET_EXPECTED(client, this->connect(sp));
    GET_EXPECTED(session, this->login(sp, client));
    CHECK_EXPECTED(this->channel_create(sp, client, session));
    CHECK_EXPECTED(this->logout(sp, client, session));
    CHECK_EXPECTED(client->close().get());
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
  this->file_download_cmd_set_.required(this->attachment_);
  this->file_download_cmd_set_.required(this->output_folder_);

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

vds::expected<std::shared_ptr<vds::http_client>> vds::vds_cmd_app::connect(
  const service_provider * sp)
{
  auto server = this->server_.value().empty() ? "tcp://localhost:8050" : this->server_.value();

  GET_EXPECTED(address, network_address::parse(server));
  GET_EXPECTED(s, tcp_network_socket::connect(sp, address));
  GET_EXPECTED(writer, s->get_output_stream(sp));

  auto client = std::make_shared<http_client>();
  CHECK_EXPECTED(client->start(sp, s, writer));

  return client;
}

vds::expected<std::string> vds::vds_cmd_app::login(
  const service_provider * sp,
  const std::shared_ptr<http_client> & client)
{
  GET_EXPECTED(response_body, client->send(
    http_message(
    "GET",
    "/api/login?login=" + url_encode::encode(this->user_login_.value())
    + "&password=" + url_encode::encode(this->user_password_.value())), 
    [](http_message response) -> async_task<expected<void>> {

    http_response login_response(std::move(response));

    if (login_response.code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Login failed");
    }

    return expected<void>();
  }).get());


  GET_EXPECTED(body, json_parser::parse("/api/login", response_body));

  const auto body_object = dynamic_cast<const json_object *>(body.get());

  std::string value;
  CHECK_EXPECTED(body_object->get_property("state", value));

  if ("successful" != value) {
    return vds::make_unexpected<std::runtime_error>("Login failed " + value);
  }

  std::string session;
  CHECK_EXPECTED(body_object->get_property("session", session));
  return session;
}

vds::expected<void> vds::vds_cmd_app::logout(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session) {

  return client->send(
    http_message(
      "POST",
      "/api/logout?session=" + url_encode::encode(session)),
    [](http_message response) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    if (http_response(response).code() != http_response::HTTP_OK && http_response(response).code() != http_response::HTTP_Found) {
      return vds::make_unexpected<std::runtime_error>("Logout failed " + http_response(response).comment());
    }

    return expected<std::shared_ptr<stream_output_async<uint8_t>>>();
  }).get();
}


vds::expected<void> vds::vds_cmd_app::upload_file(
  const service_provider * sp,
  const std::shared_ptr<http_client> & client,
  const std::string & session) {
  filename fn(this->attachment_.value());
  return this->upload_file(
    sp,
    client,
    session,
    fn,
    fn.name(),
    const_data_buffer());
}

vds::expected<void> vds::vds_cmd_app::upload_file(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session,
  const filename& fn,
  const std::string& name,
  const_data_buffer file_hash_) {

  http_multipart_request request(
    "POST",
    "/api/upload?session=" + url_encode::encode(session));

  request.add_string("channel_id", this->channel_id_.value());
  if (!this->message_.value().empty()) {
    request.add_string("message", this->message_.value());
  }

  if (0 == file_hash_.size()) {
    GET_EXPECTED(file_hash, hash::create(hash::sha256()));

    file f;
    CHECK_EXPECTED(f.open(fn, file::file_mode::open_read));
    for (;;) {
      uint8_t buffer[1024];
      GET_EXPECTED(readed, f.read(buffer, sizeof(buffer)));

      if (readed == 0) {
        break;
      }

      CHECK_EXPECTED(file_hash.update(buffer, readed));
    }
    CHECK_EXPECTED(f.close());

    CHECK_EXPECTED(file_hash.final());

    file_hash_ = file_hash.signature();
  }

  std::list<std::string> headers;
  headers.push_back("X-VDS-SHA256:" + base64::from_bytes(file_hash_));

  auto mimetype = http_mimetype::mimetype(fn);
  if (mimetype.empty()) {
    mimetype = "application/octet-stream";
  }
  
  CHECK_EXPECTED(request.add_file("attachment", fn, name, mimetype, headers));

  GET_EXPECTED(response_body, request.send(client, [](http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Upload failed " + http_response(response).comment());
    }

    return expected<void>();
  }).get());

  GET_EXPECTED(body, json_parser::parse("/api/upload", response_body));

  const auto body_object = dynamic_cast<const json_object *>(body.get());

  std::string value;
  CHECK_EXPECTED(body_object->get_property("state", value));

  if ("successful" != value) {
    return vds::make_unexpected<std::runtime_error>("Upload failed " + value);
  }

  return expected<void>();

}

vds::expected<void> vds::vds_cmd_app::download_file(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session) {
  filename fn(foldername(this->output_folder_.value()), this->attachment_.value());
  CHECK_EXPECTED(fn.contains_folder().create());

  return this->download_file(
    sp,
    client,
    session,
    fn,
    this->attachment_.value(),
    const_data_buffer());

}

vds::expected<void> vds::vds_cmd_app::download_file(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session,
  const filename & fn,
  const std::string & file_name,
  const const_data_buffer& file_id) {

  GET_EXPECTED(tmp_file, file_stream_output_async::create_tmp(sp));
  GET_EXPECTED(h, hash_stream_output_async::create(hash::sha256(), tmp_file));

  auto result = client->send(
    http_message(
      "GET",
      "/api/download?session=" + url_encode::encode(session)
      + "&channel_id=" + url_encode::encode(this->channel_id_.value())
      + "&object_id=" + url_encode::encode(base64::from_bytes(file_id))
      + "&file_name=" + url_encode::encode(file_name)
    ),
    [this, sp, h](http_message response) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("File download failed: " + http_response(response).comment());
    }

    return expected<std::shared_ptr<stream_output_async<uint8_t>>>(h);
  }).get();

  if(result.has_error()) {
    (void)file::delete_file(tmp_file->target().name());
    return std::move(result);

  }

  if(0 < file_id.size() && file_id != h->signature()) {
    (void)file::delete_file(tmp_file->target().name());
    return vds::make_unexpected<std::runtime_error>("File download failed: file is corrupted");
  }

  if(file::exists(fn)) {
    CHECK_EXPECTED(file::delete_file(fn));
  }

  CHECK_EXPECTED(file::move(tmp_file->target().name(), fn));

  return expected<void>();
}

struct sync_config {
  std::string target;
  std::map<std::string, std::string> file_map;
};

vds::expected<void> vds::vds_cmd_app::sync_files(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session) {

  sync_config cfg;
  filename config_file(foldername(this->output_folder_.value()), ".vds.config");
  if (file::exists(config_file)) {
    GET_EXPECTED(config_body, file::read_all_text(config_file));
    GET_EXPECTED(
      config,
      json_parser::parse(
        config_file.full_name(),
        const_data_buffer(config_body.c_str(), config_body.length())));

    auto config_obj = std::dynamic_pointer_cast<json_object>(config);
    if (config_obj) {
      CHECK_EXPECTED(config_obj->get_property("target", cfg.target));

      auto map_obj = std::dynamic_pointer_cast<json_object>(config_obj->get_property("map"));
      if (map_obj) {
        map_obj->visit([&cfg](const std::shared_ptr<json_property> & prop) {
          auto value = std::dynamic_pointer_cast<json_primitive>(prop->value());
          if (value) {
            cfg.file_map.emplace(prop->name(), value->value());
          }
        });
      }
    }
  }

  GET_EXPECTED(response_body, client->send(
    http_message(
      "GET",
      "/api/channel_feed?session=" + url_encode::encode(session)
      + "&channel_id=" + url_encode::encode(this->channel_id_.value())),
    [](http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Query channel failed " + http_response(response).comment());
    }

    return expected<void>();
  }).get());

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
  (void)sync_folder.files_recurcive([&filters, &files, config_file](const filename & fn) -> expected<bool> {

    if (fn == config_file) {
      return expected<bool>(true);
    }

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
    std::cout << "Checking local files...\n";
    for (const auto & fn : files) {
      GET_EXPECTED(name, sync_folder.relative_path(fn.first));
      auto pmap = cfg.file_map.find(name);
      if (cfg.file_map.end() != pmap) {
        name = pmap->second;
      }
      else {
        name = cfg.target + name;
      }
      auto p = name2file.find(name);
      if (name2file.end() == p) {
        std::cout << "File " << name << " not found in the network. Uploading file...\n";
        CHECK_EXPECTED(this->upload_file(
          sp,
          client,
          session,
          fn.first,
          name,
          const_data_buffer()));
      }
      else {
        std::cout << "File " << name << " found in the network. Sync file.\n";
        CHECK_EXPECTED(
          this->sync_file(
            sp,
            client,
            session,
            fn.first,
            name,
            p->second,
            true));
      }
    }
  }

  if (this->sync_style_.value().empty()
    || "default" == this->sync_style_.value()
    || "download" == this->sync_style_.value()) {
    std::cout << "Checking remove files...\n";
    for (const auto & name : name2file) {
      auto fname = name.first;
      bool is_found = false;
      for (const auto & item : cfg.file_map) {
        if (fname == item.second) {
          is_found = true;
          fname = item.first;
          break;
        }
      }

      if (!is_found && cfg.target.length() > 0 && 0 == memcmp(cfg.target.c_str(), fname.c_str(), cfg.target.length())) {
        fname = fname.substr(cfg.target.length());
      }

      filename fn(sync_folder, fname);
      auto p = files.find(fn);
      if (files.end() == p) {
        std::cout << "File " << name.first << " not found in the computed. Downloading file.\n";
        CHECK_EXPECTED(this->download_file(sp, client, session, fn, name.first, name.second.front().object_id_));
      }
      else {
        std::cout << "File " << name.first << " found in the computed. Sync file.\n";
        CHECK_EXPECTED(this->sync_file(sp, client, session, fn, name.first, name.second, false));
      }
    }
  }

  if ("upload" == this->sync_style_.value()) {
    std::cout << "Checking local files...\n";
    for (const auto & fn : files) {
      std::cout << "Remove file " << fn.first.full_name() << "\n";
      CHECK_EXPECTED(file::delete_file(fn.first));
    }
  }

  return expected<void>();
}

vds::expected<void> vds::vds_cmd_app::channel_list(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session) {

  GET_EXPECTED(response_body, client->send(
    http_message(
      "GET",
      "/api/channels?session=" + url_encode::encode(session)),
    [](http_message response) -> async_task<expected<void>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Query channels failed " + http_response(response).comment());
    }

    return expected<void>();
  }).get());
  
  return this->channel_list_out(response_body);
}

vds::expected<void> vds::vds_cmd_app::channel_create(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session) {
  return client->send(
    http_message(
      "POST",
      "/api/channels?session=" + url_encode::encode(session)
      + "&name=" + url_encode::encode(this->channel_name_.value())
      + "&type=" + url_encode::encode(this->channel_type_.value())
    ),
    [](http_message response) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    if (http_response(response).code() != http_response::HTTP_OK) {
      return vds::make_unexpected<std::runtime_error>("Create channel failed");
    }

    return expected<std::shared_ptr<stream_output_async<uint8_t>>>();
  }).get();
}

vds::expected<void> vds::vds_cmd_app::channel_list_out(const const_data_buffer & response_body) {
  if (this->output_format_.value() == "json") {
    std::cout << std::string((const char *)response_body.data(), response_body.size()) << std::endl;
  }
  else {
    GET_EXPECTED(body, json_parser::parse(
      "/api/channels",
      response_body));

    std::cout << std::setw(44) << std::left << "ID" << "|"
      << std::setw(15) << std::left << "Type" << "|"
      << "Name" << std::endl;

    auto body_array = dynamic_cast<const json_array *>(body.get());
    for (size_t i = 0; i < body_array->size(); ++i) {
      auto item = dynamic_cast<const json_object *>(body_array->get(i).get());

      std::string value;
      CHECK_EXPECTED(item->get_property("object_id", value));
      std::cout << std::setw(44) << value << "|";

      CHECK_EXPECTED(item->get_property("type", value));
      std::cout << std::setw(15) << std::left << value << "|";

      CHECK_EXPECTED(item->get_property("name", value));
      std::cout << value << std::endl;
    }
  }

  return expected<void>();
}

vds::expected<void> vds::vds_cmd_app::sync_file(
  const service_provider* sp,
  const std::shared_ptr<http_client> & client,
  const std::string& session,
  const filename& exists_files,
  const std::string& rel_name,
  const std::list<sync_file_info>& file_history,
  bool enable_upload) {

  GET_EXPECTED(h, hash::create(hash::sha256()));
  file f;
  CHECK_EXPECTED(f.open(exists_files, file::file_mode::open_read));

  for(;;) {
    uint8_t buffer[1024];
    GET_EXPECTED(readed, f.read(buffer, sizeof(buffer)));

    if(0 == readed) {
      break;
    }
    CHECK_EXPECTED(h.update(buffer, readed));
  }
  CHECK_EXPECTED(f.close());
  CHECK_EXPECTED(h.final());

  auto state_exists = false;
  for(const auto & hist : file_history) {
    if(hist.object_id_ == h.signature()) {
      state_exists = true;
      break;
    }
  }

  if(!state_exists) {
    if (enable_upload) {
      std::cout << "Local file has unique hash " << base64::from_bytes(h.signature()) << ". Uploading file...\n";
      return this->upload_file(sp, client, session, exists_files, rel_name, h.signature());
    }
  }
  else if(file_history.begin()->object_id_  == h.signature()){
    std::cout << "Local file is up to date.\n";
  }
  else {
    std::cout << "Local file has old hash " << base64::from_bytes(h.signature()) << ". Downloading new file...\n";
    return this->download_file(sp, client, session, exists_files, rel_name, file_history.begin()->object_id_);
  }

  return expected<void>();
}
