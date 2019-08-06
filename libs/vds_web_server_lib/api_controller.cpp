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
#include "http_response.h"
#include "http_request.h"
#include "private/create_message_form.h"
#include "string_format.h"
#include "current_config_dbo.h"

std::shared_ptr<vds::json_object> vds::api_controller::channel_serialize(
  const vds::user_channel & channel) {
  auto item = std::make_shared<json_object>();
  item->add_property("object_id", base64::from_bytes(channel.id()));
  item->add_property("name", channel.name());
  item->add_property("type", channel.channel_type());
  return item;
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>>
vds::api_controller::get_channels(
  const vds::service_provider * /*sp*/,
  const std::shared_ptr<user_manager> & user_mng,
  const http_message & request) {
  
  auto result = std::make_shared<json_array>();
  for(auto & channel : user_mng->get_channels()) {
    result->add(channel_serialize(*channel.second));
  }

  co_return std::static_pointer_cast<json_value>(result);
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>
vds::api_controller::create_message(
  const vds::service_provider* sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<user_manager>& user_mng,
  const http_message & request) {

  auto parser = std::make_shared<create_message_form>(sp, user_mng);

  return parser->parse(request, [output_stream, parser]() -> async_task<expected<void>> {
    CHECK_EXPECTED_ASYNC(co_await parser->complete());

    auto item = std::make_shared<json_object>();

    item->add_property("state", "successful");

    GET_EXPECTED_ASYNC(result_str, item->json_value::str());

    CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(output_stream, result_str, "application/json; charset=utf-8"));
    co_return expected<void>();
  });
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::api_controller::get_login_state(
  const vds::service_provider * sp,
  const std::string & login,
  const std::string & password,
  const std::shared_ptr<_web_server>& owner_param,
  const http_message & request_param) {

  std::shared_ptr<_web_server> owner = owner_param;
  http_message request = request_param;
  auto session_id = std::to_string(std::rand()) + "." + std::to_string(std::rand()) + "." + std::to_string(std::rand());
  auto session = std::make_shared<auth_session>(sp, session_id, login, password);
  CHECK_EXPECTED_ASYNC(co_await session->load(sp));

  auto item = std::make_shared<json_object>();

  switch (session->get_login_state()) {
  case user_manager::login_state_t::waiting:
    item->add_property("state", "100");
    break;

  case user_manager::login_state_t::login_failed:
    item->add_property("state", "failed");
    break;

  case user_manager::login_state_t::login_successful:
    item->add_property("state", "successful");
    item->add_property("session", session_id);
    item->add_property("user_name", session->user_name());

    owner->add_auth_session(session_id, session);
    break;

  default:
    co_return vds::make_unexpected<std::runtime_error>("Invalid program");
  }

  co_return item;
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::api_controller::login(
  const vds::service_provider * sp,
  const std::string & login,
  const std::string & password,
  const std::shared_ptr<_web_server>& owner_param,
  const http_message & request_param) {

  std::shared_ptr<_web_server> owner = owner_param;
  http_message request = request_param;
  auto session_id = std::to_string(std::rand()) + "." + std::to_string(std::rand()) + "." + std::to_string(std::rand());
  auto session = std::make_shared<auth_session>(sp, session_id, login, password);
  CHECK_EXPECTED_ASYNC(co_await session->load(sp));

  std::shared_ptr<json_object> item;

  for (int try_count = 1000; try_count > 0; --try_count) {
    CHECK_EXPECTED_ASYNC(co_await session->update());

    switch (session->get_login_state()) {
    case user_manager::login_state_t::waiting:
      continue;

    case user_manager::login_state_t::login_failed:
      item = std::make_shared<json_object>();
      item->add_property("state", "failed");
      break;

    case user_manager::login_state_t::login_successful:
      item = std::make_shared<json_object>();
      item->add_property("state", "successful");
      item->add_property("session", session_id);
      item->add_property("user_name", session->user_name());

      owner->add_auth_session(session_id, session);
      break;

    default:
      co_return vds::make_unexpected<std::runtime_error>("Invalid program");
    }
    break;
  }

  if (!item) {
    item = std::make_shared<json_object>();
    item->add_property("state", "failed");
  }

  co_return item;
}

vds::async_task<vds::expected<void>>
vds::api_controller::create_channel(
  const std::shared_ptr<user_manager> & user_mng,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::string & channel_type,
  const std::string & name) {

  vds::user_channel channel;
  GET_EXPECTED_VALUE_ASYNC(channel, co_await user_mng->create_channel(channel_type, name));
  GET_EXPECTED_ASYNC(body, channel_serialize(channel)->json_value::str());

  CHECK_EXPECTED_ASYNC(co_await http_response::simple_text_response(
    output_stream,
    body,
    "application/json; charset=utf-8"));

  co_return expected<void>();
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::api_controller::channel_feed(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer & channel_id) {
  auto result = std::make_shared<json_array>();
  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_transaction([user_mng, channel_id, result](database_transaction & t)->expected<void> {
    return user_mng->walk_messages(
      channel_id,
      t,
      [result](
        const transactions::user_message_transaction& message,
        const transactions::message_environment_t & message_environment)-> expected<bool> {
      auto record = std::make_shared<json_object>();
      record->add_property("message", message.message);
      record->add_property("time_point", std::to_string(message_environment.time_point_));
      auto files = std::make_shared<json_array>();
        for(const auto & file : message.files) {
          auto item = std::make_shared<json_object>();
          item->add_property("object_id", file.file_id);
          item->add_property("name", file.name);
          item->add_property("mimetype", file.mime_type);
          item->add_property("size", file.size);
          files->add(item);
        }
        record->add_property("files", files);
        result->add(record);
      return true;
    });
  }));

  co_return result;
}

vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
vds::api_controller::download_file(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const const_data_buffer& channel_id,
  const std::string & file_name,
  const const_data_buffer& file_hash) {

  return sp->get<file_manager::file_operations>()->download_file(user_mng, channel_id, file_name, file_hash);
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>>
vds::api_controller::prepare_download_file(
  const vds::service_provider * sp,
  const std::shared_ptr<vds::user_manager> & user_mng,
  const const_data_buffer& channel_id,
  const std::string& file_name,
  const const_data_buffer& file_hash) {
  vds::file_manager::file_operations::prepare_download_result_t result;
  GET_EXPECTED_VALUE_ASYNC(result, co_await sp->get<file_manager::file_operations>()->prepare_download_file(
    user_mng, channel_id, file_name, file_hash));
  co_return result.to_json();
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>>
vds::api_controller::offer_device(
  const vds::service_provider * sp,
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
    co_return vds::make_unexpected<std::system_error>(error, std::system_category(), "Get Computer Name");
  }
  result->add_property("name", hostname);
#endif// _WIN32
  GET_EXPECTED_ASYNC(root_rolder, persistence::current_user(sp));
  GET_EXPECTED_ASYNC(free_size, root_rolder.free_size());
  GET_EXPECTED_ASYNC(total_size, root_rolder.total_size());

  for(uint64_t i = 0; i < INT64_MAX; ++i){
    foldername folder(root_rolder, "storage" + std::to_string(i));
    if(folder.exist()){
      continue;
    }

    result->add_property("path", folder.local_name());
    result->add_property("free", free_size / (1024 * 1024 * 1024));
    result->add_property("size", total_size / (1024 * 1024 * 1024));
    break;
  }

  co_return result;
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::api_controller::get_statistics(
  const vds::service_provider * sp,
  const http_message & /*message*/) {

  auto statistic = co_await sp->get<server>()->get_statistic();
  CHECK_EXPECTED_ERROR_ASYNC(statistic);
  co_return statistic.value().serialize();
}

std::shared_ptr<vds::json_value> vds::api_controller::get_invite(
  user_manager& /*user_mng*/,
  const std::shared_ptr<_web_server>& /*owner*/,
  const http_message& /*message*/) {

  auto result = std::make_shared<json_object>();
  result->add_property("code", "test");
  return result;
}

vds::async_task<vds::expected<void>>
vds::api_controller::lock_device(
  const vds::service_provider * sp,
  const std::shared_ptr<vds::user_manager> &user_mng,
  const std::shared_ptr<vds::_web_server> &/*owner*/,
  const std::string &device_name,
  const std::string &local_path,
  uint64_t reserved_size) {
  if(local_path.empty() || reserved_size < 1) {
    return vds::make_unexpected<vds_exceptions::invalid_operation>();
  }

  foldername fl(local_path);
  if(fl.exist()) {
    return vds::make_unexpected<std::runtime_error>("Folder " + local_path + " already exists");
  }
  CHECK_EXPECTED(fl.create());

  return sp->get<db_model>()->async_transaction([sp, user_mng, device_name, local_path, reserved_size](database_transaction & t) -> expected<void> {
    auto client = sp->get<dht::network::client>();
    auto current_node = client->current_node_id();

    GET_EXPECTED(owner_id, user_mng->get_current_user().user_public_key()->fingerprint());

    orm::current_config_dbo t1;
    return t.execute(
        t1.insert(
            t1.node_id = current_node,
            t1.local_path = local_path,
            t1.owner_id = owner_id,
            t1.reserved_size = reserved_size * 1024 * 1024 * 1024));
  });
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::api_controller::get_session(
  const std::shared_ptr<auth_session> & session) {

  auto result = std::make_shared<json_object>();

  if(!session) {
    result->add_property("state", "fail");
  }
  else {
    result->add_property("state", "successful");
    result->add_property("session", session->id());
    result->add_property("user_name", session->user_name());
  }

  co_return result;
}

