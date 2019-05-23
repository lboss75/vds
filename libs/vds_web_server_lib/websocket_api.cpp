/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "websocket_api.h"
#include "db_model.h"
#include "transaction_log_record_dbo.h"
#include "user_manager_transactions.h"
#include "transaction_block.h"
#include "websocket.h"

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>
vds::websocket_api::open_connection(
  const vds::service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & request)
{
  return websocket::open_connection(
    sp,
    output_stream,
    request,
    [sp](
      bool /*is_binary*/,
      std::shared_ptr<websocket_output> output_stream
      ) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    return std::make_shared<collect_data>([sp, output_stream](const_data_buffer && data)->async_task<expected<void>> {
      GET_EXPECTED_ASYNC(message, json_parser::parse("Web Socket API", data));

      GET_EXPECTED_ASYNC(result, co_await process_message(sp, message));

      GET_EXPECTED_ASYNC(result_str, result->str());
      GET_EXPECTED_ASYNC(stream, co_await output_stream->start(result_str.length(), false));
      CHECK_EXPECTED_ASYNC(co_await stream->write_async((const uint8_t *)result_str.c_str(), result_str.length()));

      co_return expected<void>();
    });
  });
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::websocket_api::process_message(
  const vds::service_provider * sp,
  const std::shared_ptr<json_value> & message)
{
  auto request = std::dynamic_pointer_cast<json_object>(message);
  if (!request) {
    co_return make_unexpected<std::runtime_error>("JSON object is expected");
  }

  int id;
  GET_EXPECTED_ASYNC(isOk, request->get_property("id", id));

  if (!isOk) {
    co_return make_unexpected<std::runtime_error>("id field is expected");
  }

  auto result = co_await process_message(sp, id, request);
  if (!result.has_error()) {
    co_return std::move(result.value());
  }

  auto res = std::make_shared<json_object>();
  res->add_property("id", id);
  res->add_property("error", result.error()->what());
  co_return res;
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>>
vds::websocket_api::process_message(
  const vds::service_provider * sp,
  int id,
  const std::shared_ptr<json_object> & request) {

  auto invoke = std::dynamic_pointer_cast<json_array>(request->get_property("invoke"));

  if (!invoke) {
    co_return make_unexpected<std::runtime_error>("invoke field is expected");
  }
  auto method_name = std::dynamic_pointer_cast<json_primitive>(invoke->get(0));
  if (!method_name) {
    co_return make_unexpected<std::runtime_error>("method name is expected");
  }

  auto r = std::make_shared<json_object>();
  r->add_property("id", id);

  if ("login" == method_name->value()) {
    auto args = std::dynamic_pointer_cast<json_array>(invoke->get(1));
    if (!args) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'login'");
    }

    auto login = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!login) {
      co_return make_unexpected<std::runtime_error>("missing login argument at invoke method 'login'");
    }

    auto password = std::dynamic_pointer_cast<json_primitive>(args->get(1));
    if (!password) {
      co_return make_unexpected<std::runtime_error>("missing login argument at invoke method 'password'");
    }


    GET_EXPECTED_ASYNC(res, co_await vds::websocket_api::login(sp, login->value(), password->value()));
    r->add_property("result", res);
  }
  else {
    co_return make_unexpected<std::runtime_error>("invalid method '" + method_name->value() + "'");
  }

  co_return r;
}


vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::websocket_api::login(
  const vds::service_provider * sp,
  const std::string & username,
	const std::string & password_hash)
{
	std::shared_ptr<vds::json_object> result;

	CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
		[
			username,
			password_hash,
			&result
		](database_read_transaction & t)->expected<void> {

		orm::transaction_log_record_dbo t1;
		GET_EXPECTED(st, t.get_reader(
			t1.select(t1.id, t1.data)
			.order_by(db_desc_order(t1.order_no))));

		WHILE_EXPECTED(st.execute())
			const auto data = t1.data.get(st);
			GET_EXPECTED(block, transactions::transaction_block::create(data));

			CHECK_EXPECTED(block.walk_messages(
				[username, password_hash, &result](const transactions::root_user_transaction & message)->expected<bool> {
					if (username == message.user_name) {
						if(password_hash == message.user_credentials_key) {
							result = std::make_shared<json_object>();
							result->add_property("result", "successful");
							CHECK_EXPECTED(result->add_property("cert", message.user_cert->str()));
							result->add_property("key", base64::from_bytes(message.user_private_key));
						}
						else {
							result = std::make_shared<json_object>();
							result->add_property("result", "failed");
						}

						return false;
					}
					else {
						return true;
					}
				},
				[username, password_hash, &result](const transactions::create_user_transaction & message)->expected<bool> {
					if (username == message.user_name) {
						if (password_hash == message.user_credentials_key) {
							result = std::make_shared<json_object>();
							result->add_property("result", "successful");
							CHECK_EXPECTED(result->add_property("cert", message.user_cert->str()));
							result->add_property("key", base64::from_bytes(message.user_private_key));
						}
						else {
							result = std::make_shared<json_object>();
							result->add_property("result", "failed");
						}

						return false;
					}
					else {
						return true;
					}
				}
			));


			WHILE_EXPECTED_END()

				return expected<void>();
	}));

	co_return result;
}


