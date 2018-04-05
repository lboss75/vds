/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/index_page.h"
#include "http_simple_form_parser.h"
#include "private/api_controller.h"
#include "user_channel.h"

vds::async_task<vds::http_message> vds::index_page::create_channel(const vds::service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng, const std::shared_ptr<_web_server>& web_server,
  const http_message& message) {

  auto parser = std::make_shared<http::simple_form_parser>();

  return parser->parse(sp, message).then([sp, user_mng, web_server, parser]() -> async_task<http_message> {
    auto name = parser->values().find("channelName");
    return api_controller::create_channel(sp, user_mng, user_channel::channel_type_t::personal_channel, name->second);
  });
}
