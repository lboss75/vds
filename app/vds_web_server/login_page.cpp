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
  const vds::service_provider * sp,
  const std::shared_ptr<_web_server>& owner,
  const http_message& message) {
  auto parser = std::make_shared<http::simple_form_parser>(sp);

  co_await parser->parse(message);
  
    auto userName = parser->values().find("userName");
    auto userEmail = parser->values().find("userEmail");
    auto userPassword = parser->values().find("userPassword");

    if (
      userName == parser->values().end()
      || userEmail == parser->values().end()
      || userPassword == parser->values().end()) {
      co_return http_response::redirect("/error/?code=InvalidRegister");
    }

  auto request_body = user_manager::create_register_request(
      sp,
      userName->second,
      userEmail->second,
      userPassword->second);

      co_return http_response::file_response(
        request_body,
        "register_request.vds");
}
