///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "stdafx.h"
//#include "user_manager.h"
//#include "private/index_page.h"
//#include "http_simple_form_parser.h"
//#include "private/api_controller.h"
//#include "http_form_parser.h"
//#include "http_response.h"
//#include "http_request.h"
//#include "private/create_message_form.h"
//
//vds::async_task<vds::expected<void>> vds::index_page::create_channel(
//  const vds::service_provider * sp,
//  const std::shared_ptr<http_async_serializer> & output_stream,
//  const std::shared_ptr<user_manager> & user_mng,
//  const http_message & message) {
//
//  const auto type = message.get_parameter("type");
//  const auto name = message.get_parameter("name");
//
//  return api_controller::create_channel(
//    user_mng,
//    output_stream,
//    type,
//    name);
//}
//
//
//vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::index_page::create_message(
//  const vds::service_provider * sp,
//  const std::shared_ptr<http_async_serializer> & output_stream,
//  const std::shared_ptr<user_manager>& user_mng,
//  const http_message & request) {
//
//  auto parser = std::make_shared<create_message_form>(sp, user_mng);
//
//  return parser->parse(request, [parser, output_stream]()->vds::async_task<vds::expected<void>> {
//    CHECK_EXPECTED_ASYNC(co_await parser->complete());
//    CHECK_EXPECTED_ASYNC(co_await http_response::redirect(output_stream, "/"));
//    co_return expected<void>();
//  });
//}
//
