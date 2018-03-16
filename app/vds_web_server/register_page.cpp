/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "register_page.h"
#include "http_response.h"
#include "http_simple_form_parser.h"

vds::async_task<vds::http_message> vds::register_page::post(const service_provider& sp, const http_message& message) {
  auto parser = std::make_shared<http::simple_form_parser>();

  return parser->parse(sp, message).then([sp, parser]() {
    return http_response::redirect(sp, "/");
  });
}
