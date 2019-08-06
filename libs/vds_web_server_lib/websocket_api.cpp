/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "websocket_api.h"
#include "db_model.h"
#include "transaction_log_record_dbo.h"
#include "transaction_block.h"
#include "websocket.h"
#include "channel_message_dbo.h"
#include "dht_network_client.h"
#include "user_storage.h"

vds::websocket_api::websocket_api()
  : subscribe_timer_("WebSocket API Subscribe Timer")
{
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>>
vds::websocket_api::open_connection(
  const vds::service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & request)
{
  auto api = std::make_shared<websocket_api>();

  return websocket::open_connection(
    sp,
    output_stream,
    request,
    [sp, api](
      bool /*is_binary*/,
      std::shared_ptr<websocket_output> output_stream
      ) -> async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {

    return expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>(std::make_shared<collect_data>([sp, output_stream, api](const_data_buffer data)->async_task<expected<void>> {
      GET_EXPECTED_ASYNC(message, json_parser::parse("Web Socket API", data));

      std::list<lambda_holder_t<async_task<expected<void>>>> post_tasks;
      GET_EXPECTED_ASYNC(result, co_await api->process_message(sp, output_stream, message, post_tasks));

      GET_EXPECTED_ASYNC(result_str, result->str());
      GET_EXPECTED_ASYNC(stream, co_await output_stream->start(result_str.length(), false));
      CHECK_EXPECTED_ASYNC(co_await stream->write_async((const uint8_t *)result_str.c_str(), result_str.length()));

      for (auto & task : post_tasks) {
        CHECK_EXPECTED_ASYNC(co_await task());
      }

      co_return expected<void>();
    }));
  });
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::websocket_api::process_message(
  const vds::service_provider * sp,
  std::shared_ptr<websocket_output> output_stream,
  const std::shared_ptr<json_value> & message,
  std::list<lambda_holder_t<async_task<expected<void>>>> & post_tasks)
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

  auto result = co_await process_message(sp, output_stream, id, request, post_tasks);
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
  std::shared_ptr<websocket_output> output_stream,
  int id,
  const std::shared_ptr<json_object> & request,
  std::list<lambda_holder_t<async_task<expected<void>>>> & post_tasks) {

  std::string method_name;
  GET_EXPECTED_ASYNC(isOk, request->get_property("invoke", method_name));

  if (!isOk) {
    co_return make_unexpected<std::runtime_error>("method name is expected");
  }

  auto r = std::make_shared<json_object>();
  r->add_property("id", id);

  if ("login" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'login'");
    }

    auto login_cred = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!login_cred) {
      co_return make_unexpected<std::runtime_error>("missing login argument at invoke method 'login'");
    }

    CHECK_EXPECTED_ASYNC(co_await this->login(sp, r, login_cred->value()));
  }
  else if ("upload" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'upload'");
    }

    auto body_str = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!body_str) {
      co_return make_unexpected<std::runtime_error>("missing body argument at invoke method 'upload'");
    }

    GET_EXPECTED_ASYNC(body, base64::to_bytes(body_str->value()));

    CHECK_EXPECTED_ASYNC(co_await this->upload(sp, r, body));
  }
  else if ("devices" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'devices'");
    }

    auto owner_id_str = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!owner_id_str) {
      co_return make_unexpected<std::runtime_error>("missing body argument at invoke method 'devices'");
    }

    GET_EXPECTED_ASYNC(owner_id, base64::to_bytes(owner_id_str->value()));

    CHECK_EXPECTED_ASYNC(co_await this->devices(sp, r, owner_id));
  }
  else if ("broadcast" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'broadcast'");
    }

    auto body_str = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!body_str) {
      co_return make_unexpected<std::runtime_error>("missing body argument at invoke method 'broadcast'");
    }

    GET_EXPECTED_ASYNC(body, base64::to_bytes(body_str->value()));

    CHECK_EXPECTED_ASYNC(co_await this->broadcast(sp, r, body));
  }
  else if ("subscribe" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args || args->size() < 2) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'subscribe'");
    }

    auto cb = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!cb) {
      co_return make_unexpected<std::runtime_error>("missing cb argument at invoke method 'subscribe'");
    }

    auto method = std::dynamic_pointer_cast<json_primitive>(args->get(1));
    if (!method) {
      co_return make_unexpected<std::runtime_error>("missing method argument at invoke method 'subscribe'");
    }

    if ("channel" == method->value()) {
      auto channel_id = std::dynamic_pointer_cast<json_primitive>(args->get(2));
      if (!channel_id) {
        co_return make_unexpected<std::runtime_error>("missing channel_id argument at invoke method 'subscribe/channel'");
      }

      GET_EXPECTED_ASYNC(ch_id, base64::to_bytes(channel_id->value()));

      auto handler = this->subscribe_channel(sp, r, cb->value(), ch_id);
      post_tasks.push_back([sp, handler, output_stream]() {
        return handler->process(sp, output_stream);
      });
    }
    else {
      co_return make_unexpected<std::runtime_error>("invalid subscription '" + method->value() + "'");
    }

    if (!this->subscribe_timer_.is_started()) {
      CHECK_EXPECTED_ASYNC(this->start_timer(sp, output_stream));
    }
  }
  else {
    co_return make_unexpected<std::runtime_error>("invalid method '" + method_name + "'");
  }

  co_return r;
}

vds::async_task<vds::expected<void>> vds::websocket_api::login(
  const vds::service_provider * sp,
  std::shared_ptr<json_object> result,
  std::string login_cred)
{
	CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
		[
      login_cred,
			result
		](database_read_transaction & t)->expected<void> {

		orm::transaction_log_record_dbo t1;
		GET_EXPECTED(st, t.get_reader(
			t1.select(t1.id, t1.data)
			.order_by(db_desc_order(t1.order_no))));

		WHILE_EXPECTED(st.execute())
			const auto data = t1.data.get(st);
			GET_EXPECTED(block, transactions::transaction_block::create(data));

			CHECK_EXPECTED(block.walk_messages(
        [login_cred, &result](const transactions::create_user_transaction & message)->expected<bool> {
        if (login_cred == message.user_credentials_key) {
          
          auto res = std::make_shared<json_object>();
          CHECK_EXPECTED(res->add_property("public_key", message.user_public_key->str()));
          res->add_property("private_key", base64::from_bytes(message.user_private_key));

          result->add_property("result", res);

          return false;
        }
        else
        {
          return true;
        }
      }));


			WHILE_EXPECTED_END()

				return expected<void>();
	}));

  if (!result->get_property("result")) {
    result->add_property("error", "User not found");
  }

	co_return expected<void>();
}

vds::async_task<vds::expected<void>>
vds::websocket_api::upload(const vds::service_provider * sp, std::shared_ptr<json_object> result, const_data_buffer body)
{
    std::list<std::function<async_task<expected<void>>()>> final_tasks;
    CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_transaction(
      [
        sp,
        body,
        result,
        &final_tasks
      ](database_transaction & t)->expected<void> {
      auto network_client = sp->get<dht::network::client>();
      GET_EXPECTED(info, network_client->save(t, final_tasks, body));

      auto res = std::make_shared<json_object>();
      res->add_property("id", info.id);
      res->add_property("key", info.key);
      
      auto replicas = std::make_shared<json_array>();
      for (const auto & p : info.object_ids) {
        replicas->add(std::make_shared<json_primitive>(base64::from_bytes(p)));
      }
      res->add_property("replicas", replicas);

      result->add_property("result", res);

      return expected<void>();
    }));
     
    for (auto & p : final_tasks) {
      CHECK_EXPECTED_ASYNC(co_await p());
    }

    co_return expected<void>();
}

std::shared_ptr<vds::websocket_api::subscribe_handler> vds::websocket_api::subscribe_channel(
  const vds::service_provider * /*sp*/,
  std::shared_ptr<json_object> result,
  std::string cb,
  const_data_buffer channel_id)
{
  result->add_property("result", "true");
  auto handler = std::make_shared<subscribe_handler>(std::move(cb), std::move(channel_id));
  this->subscribe_handlers_.push_back(handler);
  return handler;
}

vds::expected<void> vds::websocket_api::start_timer(const vds::service_provider * sp, std::shared_ptr<websocket_output> output_stream)
{
  return this->subscribe_timer_.start(sp, std::chrono::seconds(10), [sp, this_ = this->weak_from_this(), target_ = std::weak_ptr<websocket_output>(output_stream)]()->async_task<expected<bool>> {
    auto pthis = this_.lock();
    if (!pthis) {
      co_return false;
    }

    for (auto & handler : pthis->subscribe_handlers_) {
      CHECK_EXPECTED_ASYNC(co_await handler->process(sp, target_));
    }

    co_return !sp->get_shutdown_event().is_shuting_down();
  });
}

vds::async_task<vds::expected<void>> vds::websocket_api::devices(
  const vds::service_provider * sp,
  std::shared_ptr<json_object> res,
  const_data_buffer owner_id)
{
  auto result = co_await user_storage::device_storage(sp);
  CHECK_EXPECTED_ASYNC(result);

  auto result_json = result.value().serialize();
 
  res->add_property("result", result_json);
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::websocket_api::broadcast(
  const vds::service_provider * sp,
  std::shared_ptr<json_object> result,
  const_data_buffer body)
{
  return sp->get<db_model>()->async_transaction(
    [
      sp,
      body,
      result
    ](database_transaction & t)->expected<void> {
    GET_EXPECTED(playback, transactions::transaction_block_builder::create(sp, t, body));

    auto network_client = sp->get<dht::network::client>();
    GET_EXPECTED(trx_id, network_client->save(sp, playback, t));
    result->add_property("result", trx_id);

    return expected<void>();
  });
}

vds::websocket_api::subscribe_handler::subscribe_handler(std::string cb, const_data_buffer channel_id)
  : cb_(std::move(cb)), channel_id_(std::move(channel_id)), last_id_(0)
{
}

vds::async_task<vds::expected<void>> vds::websocket_api::subscribe_handler::process(const vds::service_provider * sp, std::weak_ptr<websocket_output> output_stream)
{
  auto items = std::make_shared<json_array>();

  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
    [
      sp,
      this_ = this->weak_from_this(),
      output_stream,
      &items
    ](database_read_transaction & t)->expected<void> {

    auto pthis = this_.lock();
    if (!pthis) {
      return expected<void>();
    }

    orm::channel_message_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.block_id, t1.channel_id, t1.read_id, t1.write_id, t1.crypted_key, t1.crypted_data, t1.signature).where(t1.channel_id == pthis->channel_id_ && t1.id > pthis->last_id_).order_by(t1.id)));
    WHILE_EXPECTED(st.execute())
    {

      pthis->last_id_ = t1.id.get(st);

      auto item = std::make_shared<json_object>();
      item->add_property("id", t1.id.get(st));
      item->add_property("block_id", t1.block_id.get(st));
      item->add_property("channel_id", t1.channel_id.get(st));
      item->add_property("read_id", t1.read_id.get(st));
      item->add_property("write_id", t1.write_id.get(st));
      item->add_property("crypted_key", t1.crypted_key.get(st));
      item->add_property("crypted_data", t1.crypted_data.get(st));
      item->add_property("signature", t1.signature.get(st));

      items->add(item);
    }
    WHILE_EXPECTED_END()

    return expected<void>();
  }));

  if (0 < items->size()) {
    auto output_stream_ = output_stream.lock();
    if (output_stream_) {

      auto result = std::make_shared<json_object>();
      result->add_property("id", this->cb_);
      result->add_property("result", items);

      GET_EXPECTED_ASYNC(result_str, result->json_value::str());
      GET_EXPECTED_ASYNC(stream, co_await output_stream_->start(result_str.length(), false));
      CHECK_EXPECTED_ASYNC(co_await stream->write_async((const uint8_t *)result_str.c_str(), result_str.length()));
    }
  }

  co_return expected<void>();
}