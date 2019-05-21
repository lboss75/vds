/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/login_page.h"
#include "http_response.h"
#include "private/auth_session.h"
#include "private/web_server_p.h"
#include "db_model.h"
#include "register_request.h"
#include "dht_object_id.h"
#include "http_simple_form_parser.h"

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::login_page::register_request_post(
  const vds::service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const std::shared_ptr<_web_server>& /*owner*/,
  const http_message& message) {

  auto parser = std::make_shared<http::simple_form_parser>(sp);

	return parser->parse(message, [sp, output_stream, parser]() -> vds::async_task<vds::expected<void>> {

		auto userName = parser->values().find("userName");
		auto userEmail = parser->values().find("userEmail");
		auto userPassword = parser->values().find("userPassword");

		if (
			userName == parser->values().end()
			|| userEmail == parser->values().end()
			|| userPassword == parser->values().end()) {
			CHECK_EXPECTED_ASYNC(co_await http_response::redirect(output_stream, "/error/?code=InvalidRegister"));
		}
		else {
			GET_EXPECTED_ASYNC(request_body, user_manager::create_register_request(
				sp,
				userName->second,
				userEmail->second,
				userPassword->second));

			CHECK_EXPECTED_ASYNC(co_await http_response::file_response(
				output_stream,
				request_body,
				"register_request.vds"));
		}

		co_return expected<void>();
	});
}
