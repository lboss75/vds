/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/index_page.h"
#include "http_simple_form_parser.h"
#include "private/api_controller.h"
#include "user_channel.h"
#include "http_form_parser.h"
#include "file_operations.h"

vds::async_task<vds::http_message> vds::index_page::create_channel(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<http::simple_form_parser>();

  return parser->parse(sp, message).then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    auto name = parser->values().find("channelName");
    return api_controller::create_channel(sp, user_mng, user_channel::channel_type_t::personal_channel, name->second);
  });
}

class create_message_form : public vds::http::form_parser<create_message_form> {
public:
  create_message_form(
    const vds::service_provider & sp,
    const std::shared_ptr<vds::user_manager>& user_mng)
  : sp_(sp), user_mng_(user_mng) {
    
  }

  void on_field(const simple_field_info & field) {
    if(field.name == "channel_id") {
      this->channel_id_ = vds::base64::to_bytes(field.value);
    }
    else
    if (field.name == "message") {
      //send_message();
    }
    else {
      throw std::runtime_error("Invalid field " + field.name);
    }
  }

  vds::async_task<> on_file(const file_info & file) {
    return this->sp_.get<vds::file_manager::file_operations>()->upload_file(
        this->sp_,
        this->user_mng_,
        this->channel_id_,
        file.file_name,
        file.mimetype,
        file.stream);
  }

private:
  vds::service_provider sp_;
  std::shared_ptr<vds::user_manager> user_mng_;
  vds::const_data_buffer channel_id_;
};

vds::async_task<vds::http_message> vds::index_page::create_message(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<create_message_form>(sp, user_mng);

  return parser->parse(sp, message).then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    //auto name = parser->values().find("channelName");
    //return api_controller::create_channel(sp, user_mng, user_channel::channel_type_t::personal_channel, name->second);
    return vds::async_task<vds::http_message>::result(
      http_response::simple_text_response(
        sp,
        "{}",
        "application/json; charset=utf-8"));
  });
}
