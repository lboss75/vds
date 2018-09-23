/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/login_page.h"
#include "http_simple_form_parser.h"
#include "http_response.h"
#include "private/auth_session.h"
#include "private/web_server_p.h"
#include "db_model.h"
#include "register_request.h"
#include "dht_object_id.h"

vds::async_task<vds::http_message> vds::login_page::register_request_post(
  const service_provider& sp,
  const std::shared_ptr<_web_server>& owner,
  const http_message& message) {
  auto parser = std::make_shared<http::simple_form_parser>();

  return parser->parse(sp, message).then([sp, owner, parser]() -> vds::async_task<http_message> {
    auto userName = parser->values().find("userName");
    auto userEmail = parser->values().find("userEmail");
    auto userPassword = parser->values().find("userPassword");

    if (
      userName == parser->values().end()
      || userEmail == parser->values().end()
      || userPassword == parser->values().end()) {
      return vds::async_task<http_message>::result(
        http_response::redirect(sp, "/error/?code=InvalidRegister"));
    }

    return user_manager::create_register_request(
      sp,
      userName->second,
      userEmail->second,
      userPassword->second).then([sp](const const_data_buffer & request_id){

      return http_response::redirect(sp, "/register_request?id=" + url_encode::encode(base64::from_bytes(request_id)));
    });
  });
}
