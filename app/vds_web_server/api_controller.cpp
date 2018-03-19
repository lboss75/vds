//
// Created by vadim on 18.03.18.
//

#include "json_object.h"
#include "user_manager.h"
#include "private/api_controller.h"

vds::async_task<vds::http_message>
vds::api_controller::get_channels(
    const vds::service_provider &sp,
    user_manager & user_mng,
    const std::shared_ptr<vds::_web_server> &owner,
    const vds::http_message &message) {
  auto result = std::make_shared<json_array>();
  for(auto & channel : user_mng.get_channels()) {
    auto item = std::make_shared<json_object>();
    item->add_property("id", base64::from_bytes(channel.id()));
    item->add_property("name", channel.name());
    result->add(item);
  }

  return vds::async_task<vds::http_message>::result(
      http_response::simple_text_response(
          sp,
          result->json_value::str(),
          "application/json; charset=utf-8"));

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
