//
// Created by vadim on 18.03.18.
//
#include "stdafx.h"
#include "device_config_dbo.h"
#include "dht_network_client.h"
#include "json_object.h"
#include "user_manager.h"
#include "private/api_controller.h"
#include "http_simple_form_parser.h"
#include "db_model.h"
#include "file_operations.h"
#include "private/auth_session.h"
#include "dht_object_id.h"
#include "register_request.h"
#include "vds_exceptions.h"
#include "member_user.h"
#include "device_record_dbo.h"
#include "server.h"

std::shared_ptr<vds::json_object> vds::api_controller::channel_serialize(
  const vds::user_channel & channel) {
  auto item = std::make_shared<json_object>();
  item->add_property("object_id", base64::from_bytes(channel.id()));
  item->add_property("name", channel.name());
  return item;
}

std::shared_ptr<vds::json_value>
vds::api_controller::get_channels(
    const vds::service_provider &sp,
    user_manager & user_mng,
    const std::shared_ptr<vds::_web_server> &owner,
    const vds::http_message &message) {
  auto result = std::make_shared<json_array>();
  for(auto & channel : user_mng.get_channels()) {
    result->add(channel_serialize(*channel.second));
  }

  return std::static_pointer_cast<json_value>(result);
}

std::future<vds::http_message> vds::api_controller::get_login_state(
  const service_provider& sp,
  const std::string & login,
  const std::string & password,
  const std::shared_ptr<_web_server>& owner,
  const http_message& message) {

  auto session_id = std::to_string(std::rand()) + "." + std::to_string(std::rand()) + "." + std::to_string(std::rand());
  auto session = std::make_shared<auth_session>(login, password);
  co_await session->load(sp);

  auto item = std::make_shared<json_object>();

  switch (session->get_login_state()) {
  case user_manager::login_state_t::waiting:
    item->add_property("state", "100");
    break;

  case user_manager::login_state_t::login_failed:
    item->add_property("state", "failed");
    break;

  case user_manager::login_state_t::login_sucessful:
    item->add_property("state", "sucessful");
    item->add_property("session", session_id);
    item->add_property("user_name", session->user_name());

    owner->add_auth_session(session_id, session);
    break;

  default:
    throw std::runtime_error("Invalid program");
  }

  co_return http_response::simple_text_response(
    item->json_value::str(),
    "application/json; charset=utf-8");
}

std::future<vds::http_message>
vds::api_controller::create_channel(
  const vds::service_provider &sp,
  const std::shared_ptr<vds::user_manager> &user_mng,
  const std::string & name) {

  auto channel = co_await user_mng->create_channel(sp, name);
  
  co_return http_response::simple_text_response(
        channel_serialize(channel)->json_value::str(),
        "application/json; charset=utf-8");
}

std::future<std::shared_ptr<vds::json_value>> vds::api_controller::channel_feed(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer & channel_id) {
  auto result = std::make_shared<json_array>();
  co_await sp.get<db_model>()->async_transaction(sp, [sp, user_mng, channel_id, result](database_transaction & t)->bool {
    user_mng->walk_messages(
      sp,
      channel_id,
      t,
      [result](const transactions::user_message_transaction& message)-> bool {
      auto record = std::make_shared<json_object>();
      record->add_property("message", message.message);
      auto files = std::make_shared<json_array>();
        for(const auto & file : message.files) {
          auto item = std::make_shared<json_object>();
          item->add_property("object_id", base64::from_bytes(file.file_id));
          item->add_property("name", file.name);
          item->add_property("mimetype", file.mime_type);
          item->add_property("size", file.size);
          files->add(item);
        }
        record->add_property("files", files);
        result->add(record);
      return true;
    });
    return true;
  });

  co_return std::static_pointer_cast<json_value>(result);
}

std::future<vds::file_manager::file_operations::download_result_t>
vds::api_controller::download_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer& channel_id,
  const const_data_buffer& file_hash,
  const std::shared_ptr<stream_output_async<uint8_t>> & output_stream) {

  co_return co_await sp.get<file_manager::file_operations>()->download_file(sp, user_mng, channel_id, file_hash, output_stream);
}

std::future<std::shared_ptr<vds::json_value>>
vds::api_controller::user_devices(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::user_manager> &user_mng,
    const std::shared_ptr<vds::_web_server> &owner) {
  auto result = std::make_shared<json_array>();

  co_await sp.get<db_model>()->async_read_transaction(sp, [sp, user_mng, result](database_read_transaction & t){
    auto client = sp.get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    orm::device_record_dbo t2;
    db_value<uint64_t> used_size;
    auto st = t.get_reader(
        t1.select(
            t1.name,
            t1.node_id,
            t1.local_path,
            t1.reserved_size,
            db_sum(t2.data_size).as(used_size))
        .left_join(t2, t2.local_path == t1.local_path && t2.node_id == t1.node_id)
        .where(t1.owner_id == user_mng->get_current_user().user_certificate()->subject())
        .group_by(t1.name, t1.node_id, t1.local_path, t1.reserved_size));
    while(st.execute()){
      auto item = std::make_shared<json_object>();
      item->add_property("name", t1.name.get(st));
      item->add_property("local_path", t1.local_path.get(st));
      item->add_property("reserved_size", t1.reserved_size.get(st));
      item->add_property("used_size", std::to_string(used_size.get(st)));
      item->add_property("free_size", std::to_string(foldername(t1.local_path.get(st)).free_size()));
      item->add_property(
          "current",
          (t1.node_id.get(st) == current_node) ? "true" : "false");

      result->add(item);
    }
  });
  
  co_return std::static_pointer_cast<json_value>(result);
}

std::future<std::shared_ptr<vds::json_value>>
vds::api_controller::offer_device(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::user_manager> &user_mng,
    const std::shared_ptr<vds::_web_server> &owner) {
  auto result = std::make_shared<json_object>();
#ifndef _WIN32
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  result->add_property("name", hostname);
#else// _WIN32
  CHAR hostname[256];
  DWORD bufCharCount = sizeof(hostname) / sizeof(hostname[0]);
  if(!GetComputerNameA(hostname, &bufCharCount)){
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Get Computer Name");
  }
  result->add_property("name", hostname);
#endif// _WIN32
  const foldername root_rolder(persistence::current_user(sp), ".vds");
  const auto free_size = root_rolder.free_size() / (1024 * 1024 * 1024);
  const auto total_size = root_rolder.total_size() / (1024 * 1024 * 1024);

  for(uint64_t i = 0; i < INT64_MAX; ++i){
    foldername folder(root_rolder, "storage" + std::to_string(i));
    if(folder.exist()){
      continue;
    }

    result->add_property("path", folder.local_name());
    result->add_property("free", free_size);
    result->add_property("size", total_size);
    break;
  }

  co_return result;
}

std::future<std::shared_ptr<vds::json_value>> vds::api_controller::get_statistics(
  const service_provider& sp,
  const std::shared_ptr<_web_server>& owner,
  const http_message& message) {

  auto statistic = co_await sp.get<server>()->get_statistic(sp);
  co_return statistic.serialize();
}

std::shared_ptr<vds::json_value> vds::api_controller::get_invite(const service_provider& sp, user_manager& user_mng,
  const std::shared_ptr<_web_server>& owner, const http_message& message) {

  auto result = std::make_shared<json_object>();
  result->add_property("code", "test");
  return result;
}

std::future<void>
vds::api_controller::lock_device(const vds::service_provider &sp, const std::shared_ptr<vds::user_manager> &user_mng,
                                 const std::shared_ptr<vds::_web_server> &owner, const std::string &device_name,
                                 const std::string &local_path, uint64_t reserved_size) {
  if(local_path.empty() || reserved_size < 1) {
    throw vds_exceptions::invalid_operation();
  }

  foldername fl(local_path);
  if(fl.exist()) {
    throw std::runtime_error("Folder " + local_path + " already exists");
  }
  fl.create();

  return sp.get<db_model>()->async_transaction(sp, [sp, user_mng, device_name, local_path, reserved_size](database_transaction & t) {
    auto client = sp.get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    t.execute(
        t1.insert(
            t1.node_id = current_node,
            t1.local_path = local_path,
            t1.owner_id = user_mng->get_current_user().user_certificate()->subject(),
            t1.name = device_name,
            t1.reserved_size = reserved_size * 1024 * 1024 * 1024));
  });
}

std::future<std::shared_ptr<vds::json_value>>
vds::api_controller::get_register_requests(
  const vds::service_provider &sp,
  const std::shared_ptr<vds::_web_server> &owner) {

  auto result = std::make_shared<json_array>();

  co_await sp.get<db_model>()->async_transaction(sp, [sp, result](database_transaction & t) {
    orm::register_request t1;
    auto st = t.get_reader(t1.select(t1.id, t1.name, t1.email, t1.create_time));
    while(st.execute()){
      auto item = std::make_shared<json_object>();
      item->add_property("object_id", t1.id.get(st));
      item->add_property("name", t1.name.get(st));
      item->add_property("email", t1.email.get(st));
      item->add_property("create_time", t1.create_time.get(st));
      result->add(item);
    }
  });
  
  co_return std::static_pointer_cast<json_value>(result);
}

std::future<std::shared_ptr<vds::json_value>>
vds::api_controller::get_register_request(
  const vds::service_provider &sp,
  const std::shared_ptr<vds::_web_server> &owner,
  const const_data_buffer & request_id) {

  auto result = std::make_shared<json_array>();

  co_await sp.get<db_model>()->async_transaction(sp, [sp, result, request_id](database_transaction & t) {
    orm::register_request t1;
    auto st = t.get_reader(t1.select(t1.id, t1.name, t1.email, t1.create_time).where(t1.id == request_id));
    while (st.execute()) {
      auto item = std::make_shared<json_object>();
      item->add_property("object_id", t1.id.get(st));
      item->add_property("name", t1.name.get(st));
      item->add_property("email", t1.email.get(st));
      item->add_property("create_time", t1.create_time.get(st));
      result->add(item);
    }
  });

  co_return std::static_pointer_cast<json_value>(result);
}

std::future<vds::const_data_buffer> vds::api_controller::get_register_request_body(
  const service_provider& sp,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer & request_id) {

  auto result = std::make_shared<const_data_buffer>();

  co_await sp.get<db_model>()->async_read_transaction(sp, [sp, request_id, result](database_read_transaction & t) {
    orm::register_request t1;
    auto st = t.get_reader(t1.select(t1.data).where(t1.id == request_id));
    if (!st.execute()) {
      throw vds::vds_exceptions::not_found();
    }

    *result = t1.data.get(st);

    return true;
  });

  co_return *result;
}

std::future<vds::http_message> vds::api_controller::get_session(
  const service_provider& sp,
  const std::shared_ptr<_web_server>& owner,
  const std::string& session_id) {
  auto session = owner->get_session(sp, session_id);

  auto result = std::make_shared<json_object>();

  if(!session) {
    result->add_property("state", "fail");
  }
  else {
    result->add_property("state", "sucessful");
    result->add_property("session", session_id);
    result->add_property("user_name", session->user_name());
  }

  co_return http_response::simple_text_response(
      result->json_value::str(),
      "application/json; charset=utf-8");
}

std::future<vds::http_message> vds::api_controller::logout(const service_provider& sp,
  const std::shared_ptr<_web_server>& owner, const std::string & session_id) {
  owner->kill_session(sp, session_id);

  co_return http_response::redirect("/");
}
