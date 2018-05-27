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

std::shared_ptr<vds::json_object> vds::api_controller::channel_serialize(
  const vds::user_channel & channel) {
  auto item = std::make_shared<json_object>();
  item->add_property("id", base64::from_bytes(channel.id()));
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
    result->add(channel_serialize(channel));
  }

  return std::static_pointer_cast<json_value>(result);
}

vds::async_task<vds::http_message> vds::api_controller::get_login_state(
  const service_provider& sp,
  const std::string & login,
  const std::string & password,
  const std::shared_ptr<_web_server>& owner,
  const http_message& message) {

  return sp.get<dht::network::client>()->restore_async(
    sp,
    dht::dht_object_id::user_credentials_to_key(login, password))
    .then([sp, owner, login, password](uint8_t percent, const const_data_buffer & crypted_private_key) {
    if (!crypted_private_key) {
      auto item = std::make_shared<json_object>();
      item->add_property("state", std::to_string(percent));

      return vds::async_task<vds::http_message>::result(
        http_response::simple_text_response(
          sp,
          item->json_value::str(),
          "application/json; charset=utf-8"));
    }

    auto session_id = guid::new_guid().str();
    auto session = std::make_shared<auth_session>(login, password);
    return session->load(sp, crypted_private_key).then([sp, session_id, session, owner]() {
      owner->add_auth_session(session_id, session);

      auto item = std::make_shared<json_object>();
      item->add_property("state", "100");
      item->add_property("url", "/");

      const auto body = item->json_value::str();

      http_response response(http_response::HTTP_OK, "OK");
      response.add_header("Content-Type", "application/json; charset=utf-8");
      response.add_header("Content-Length", std::to_string(body.length()));
      response.add_header("Set-Cookie", "Auth=" + session_id + "; Path=/");

      auto result = response.create_message(sp);
      auto buffer = std::make_shared<std::string>(body);
      result.body()->write_async((const uint8_t *)buffer->c_str(), buffer->length())
        .execute(
          [sp, result, buffer](const std::shared_ptr<std::exception> & ex) {
        if (!ex) {
          result.body()->write_async(nullptr, 0).execute(
            [sp](const std::shared_ptr<std::exception> & ex) {
            if (ex) {
              sp.unhandled_exception(ex);
            }
          });
        }
        else {
          sp.unhandled_exception(ex);
        }
      });

      return result;
    });

  });
}

vds::async_task<vds::http_message>
vds::api_controller::create_channel(
  const vds::service_provider &sp,
  const std::shared_ptr<vds::user_manager> &user_mng,
  user_channel::channel_type_t channel_type,
  const std::string & name) {

  return user_mng->create_channel(sp, channel_type, name).then([sp](const vds::user_channel & channel) {
    return vds::async_task<vds::http_message>::result(
      http_response::simple_text_response(
        sp,
        channel_serialize(channel)->json_value::str(),
        "application/json; charset=utf-8"));
  });
}

vds::async_task<std::shared_ptr<vds::json_value>> vds::api_controller::channel_feed(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer & channel_id) {
  auto result = std::make_shared<json_array>();
  return sp.get<db_model>()->async_transaction(sp, [sp, user_mng, channel_id, result](database_transaction & t)->bool {
    user_mng->walk_messages(
      sp,
      channel_id,
      t,
      [result](const transactions::file_add_transaction& message)-> bool {
      auto record = std::make_shared<json_object>();
      record->add_property("id", base64::from_bytes(message.total_hash()));
      record->add_property("name", message.name());
      record->add_property("mimetype", message.mimetype());
      record->add_property("size", message.total_size());
      result->add(record);
      return true;
    });
    return true;
  }).then([result]() {
    return std::static_pointer_cast<json_value>(result);
  });
}

vds::async_task<
  std::string /*content_type*/,
  std::string /*filename*/,
  size_t /*body_size*/,
  std::shared_ptr<vds::continuous_buffer<uint8_t>> /*output_stream*/>
vds::api_controller::download_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer& channel_id,
  const const_data_buffer& file_hash) {

  return sp.get<file_manager::file_operations>()->download_file(sp, user_mng, channel_id, file_hash).then(
    [](const file_manager::file_operations::download_result_t & result) -> async_task<std::string, std::string, size_t, std::shared_ptr<vds::continuous_buffer<uint8_t>>>{
    return async_task<std::string, std::string, size_t, std::shared_ptr<continuous_buffer<uint8_t>>>::result(
      result.mime_type,
      result.name,
      result.size,
      result.output_stream);
  });
}

vds::async_task<std::shared_ptr<vds::json_value>>
vds::api_controller::user_devices(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::user_manager> &user_mng,
    const std::shared_ptr<vds::_web_server> &owner) {
  auto result = std::make_shared<json_array>();
  return sp.get<db_model>()->async_read_transaction(sp, [sp, user_mng, result](database_transaction & t){
    auto client = sp.get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    auto st = t.get_reader(
        t1.select(
            t1.id,
            t1.name,
            t1.reserved_size,
            t1.free_size)
        .where(t1.owner_id == base64::from_bytes(user_mng->dht_user_id())));
    while(st.execute()){
      auto item = std::make_shared<json_object>();
      item->add_property("id", t1.id.get(st));
      item->add_property("name", t1.name.get(st));
      item->add_property("reserved_size", t1.reserved_size.get(st));
      item->add_property("free_size", t1.free_size.get(st));
      item->add_property("current", (t1.id.get(st) == base64::from_bytes(current_node)) ? "true" : "false");
      result->add(item);
    }
  }).then([result](){
    return std::static_pointer_cast<json_value>(result);
  });
}

vds::async_task<std::shared_ptr<vds::json_value>>
vds::api_controller::offer_device(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::user_manager> &user_mng,
    const std::shared_ptr<vds::_web_server> &owner) {
  auto result = std::make_shared<json_object>();
#ifndef _WIN32
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  result->add_property("name", hostname);

  struct statvfs buf;
  if(0 != statvfs(persistence::current_user(sp).local_name().c_str(), &buf)){
    auto error = errno;
    throw std::system_error(error, std::system_category(), "Get Free Size");
  }
  result->add_property("free", buf.f_bfree * buf.f_frsize / (1024 * 1024 * 1024));
  result->add_property("size", buf.f_bsize * buf.f_frsize / (1024 * 1024 * 1024));
#else// _WIN32
  CHAR hostname[256];
  DWORD bufCharCount = sizeof(hostname) / sizeof(hostname[0]);
  if(!GetComputerNameA(hostname, &bufCharCount)){
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Get Computer Name");
  }
  result->add_property("name", hostname);

  ULARGE_INTEGER freeBytesAvailable;
  ULARGE_INTEGER totalNumberOfBytes;
  if(!GetDiskFreeSpaceExA(
    persistence::current_user(sp).local_name().c_str(),
    &freeBytesAvailable,
    &totalNumberOfBytes,
    NULL)){
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Get Free Size");
  }
  result->add_property("free", freeBytesAvailable.QuadPart / (1024 * 1024 * 1024));
  result->add_property("size", totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024));
#endif// _WIN32
  return vds::async_task<std::shared_ptr<vds::json_value>>::result(result);
}

std::shared_ptr<vds::json_value> vds::api_controller::get_statistics(const service_provider& sp,
  const std::shared_ptr<_web_server>& owner, const http_message& message) {

  http_request request(message);

  route_statistic statistic;
  sp.get<dht::network::client>()->get_route_statistics(statistic);
  return statistic.serialize(request.get_parameter("all") == "true");
}

std::shared_ptr<vds::json_value> vds::api_controller::get_invite(const service_provider& sp, user_manager& user_mng,
  const std::shared_ptr<_web_server>& owner, const http_message& message) {

  auto result = std::make_shared<json_object>();
  result->add_property("code", "test");
  return result;
}

vds::async_task<>
vds::api_controller::lock_device(
    const vds::service_provider &sp,
    const std::shared_ptr<vds::user_manager> &user_mng,
    const std::shared_ptr<vds::_web_server> &owner,
    const std::string &device_name,
    uint64_t reserved_size) {
  return sp.get<db_model>()->async_transaction(sp, [sp, user_mng, device_name, reserved_size](database_transaction & t) {
    auto client = sp.get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    t.execute(
        t1.insert(
            t1.id = base64::from_bytes(current_node),
            t1.owner_id = base64::from_bytes(user_mng->dht_user_id()),
            t1.name = device_name,
            t1.reserved_size = reserved_size,
            t1.free_size = reserved_size));
  });
}
