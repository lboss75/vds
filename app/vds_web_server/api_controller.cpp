//
// Created by vadim on 18.03.18.
//

#include "json_object.h"
#include "user_manager.h"
#include "private/api_controller.h"
#include "http_simple_form_parser.h"
#include "db_model.h"
#include "file_operations.h"

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

vds::async_task<vds::http_message> vds::api_controller::get_login_state(const service_provider& sp,
  user_manager& user_mng, const std::shared_ptr<_web_server>& owner, const http_message& message) {

  return user_mng.update(sp).then([sp, &user_mng]() -> async_task<http_message> {

    auto item = std::make_shared<json_object>();
    switch (user_mng.get_login_state()) {
    case security_walker::login_state_t::waiting_channel:
      item->add_property("state", "waiting");
      break;

    case security_walker::login_state_t::login_sucessful:
      item->add_property("state", "sucessful");
      break;

    case security_walker::login_state_t::login_failed:
      item->add_property("state", "failed");
      break;

    default:
      throw std::runtime_error("Invalid operation");
    }

    return vds::async_task<vds::http_message>::result(
      http_response::simple_text_response(
        sp,
        item->json_value::str(),
        "application/json; charset=utf-8"));
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
  const std::string & /*content_type*/,
  size_t /*body_size*/,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & /*output_stream*/>
vds::api_controller::download_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::shared_ptr<_web_server>& owner,
  const const_data_buffer& channel_id,
  const const_data_buffer& file_hash) {

  return sp.get<file_manager::file_operations>()->download_file(sp, user_mng, channel_id, file_hash).then(
    [](const file_manager::file_operations::download_result_t & result) -> async_task<const std::string &, size_t, const std::shared_ptr<vds::continuous_buffer<uint8_t>> &>{
    return async_task<const std::string &, size_t, const std::shared_ptr<continuous_buffer<uint8_t>> &>::result(
      result.mime_type,
      (size_t)result.size,
      result.output_stream);
  });
}
