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
#include "dht_network.h"
#include "device_record_dbo.h"
#include "../private/dht_network_client_p.h"
#include "transaction_block_builder.h"
#include "node_storage_dbo.h"
#include "local_data_dbo.h"
#include "server_api.h"

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
    if (!args || args->size() < 2) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'login'");
    }

    auto login = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!login) {
      co_return make_unexpected<std::runtime_error>("missing login argument at invoke method 'login'");
    }

    CHECK_EXPECTED_ASYNC(co_await this->login(sp, r, login->value()));
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
  else if ("download" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args || args->size() != dht::network::service::GENERATE_HORCRUX) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'download'");
    }

    std::vector<const_data_buffer> object_ids;
    for(decltype(args->size()) i = 0; i < args->size(); ++i){
      auto id_str = std::dynamic_pointer_cast<json_primitive>(args->get(i));
      if (!id_str) {
        co_return make_unexpected<std::runtime_error>("missing argument at invoke method 'download'");
      }

      GET_EXPECTED_ASYNC(obj_id, base64::to_bytes(id_str->value()));

      object_ids.push_back(obj_id);
    }

    CHECK_EXPECTED_ASYNC(co_await this->download(sp, r, object_ids));
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
    if (!body_str || body_str->value().empty()) {
      co_return make_unexpected<std::runtime_error>("missing body argument at invoke method 'broadcast'");
    }

    GET_EXPECTED_ASYNC(body, base64::to_bytes(body_str->value()));

    CHECK_EXPECTED_ASYNC(co_await this->broadcast(sp, r, body));
  }
  else if ("get_channel_messages" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args || args->size() < 1) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'get_channel_messages'");
    }

    auto channel_id_str = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!channel_id_str) {
      co_return make_unexpected<std::runtime_error>("missing channel_id argument at invoke method 'get_channel_messages'");
    }

    GET_EXPECTED_ASYNC(channel_id, base64::to_bytes(channel_id_str->value()));

#undef max
    int64_t last_id = std::numeric_limits<int64_t>::max();
    int limit = 1000;

    if (1 < args->size()) {
      auto last_id_str = std::dynamic_pointer_cast<json_primitive>(args->get(1));
      if (!last_id_str) {
        co_return make_unexpected<std::runtime_error>("invalid last_id argument at invoke method 'get_channel_messages'");
      }
#ifdef _WIN32
      last_id = _atoi64(last_id_str->value().c_str());
#else
        last_id = atoll(last_id_str->value().c_str());
#endif
    }

    if (2 < args->size()) {
      auto limit_str = std::dynamic_pointer_cast<json_primitive>(args->get(2));
      if (!limit_str) {
        co_return make_unexpected<std::runtime_error>("invalid limit argument at invoke method 'get_channel_messages'");
      }

      limit = atoi(limit_str->value().c_str());
      if (limit > 1000) {
        limit = 1000;
      }
    }

    CHECK_EXPECTED_ASYNC(co_await this->get_channel_messages(sp, r, channel_id, last_id, limit));
  }
  else if ("allocate_storage" == method_name) {
    auto args = std::dynamic_pointer_cast<json_array>(request->get_property("params"));
    if (!args || args->size() < 3) {
      co_return make_unexpected<std::runtime_error>("invalid arguments at invoke method 'allocate_storage'");
    }

    auto public_key_str = std::dynamic_pointer_cast<json_primitive>(args->get(0));
    if (!public_key_str || public_key_str->value().empty()) {
      co_return make_unexpected<std::runtime_error>("missing public_key argument at invoke method 'allocate_storage'");
    }

    GET_EXPECTED_ASYNC(public_key_der, base64::to_bytes(public_key_str->value()));
    GET_EXPECTED_ASYNC(public_key, asymmetric_public_key::parse_der(public_key_der));

    auto folder = std::dynamic_pointer_cast<json_primitive>(args->get(1));
    if (!folder || folder->value().empty()) {
      co_return make_unexpected<std::runtime_error>("missing folder argument at invoke method 'allocate_storage'");
    }

    auto size_str = std::dynamic_pointer_cast<json_primitive>(args->get(2));
    if (!size_str || size_str->value().empty()) {
      co_return make_unexpected<std::runtime_error>("missing size argument at invoke method 'allocate_storage'");
    }

    auto size = atol(size_str->value().c_str());

    CHECK_EXPECTED_ASYNC(co_await this->allocate_storage(sp, r, std::move(public_key), foldername(folder->value()), size));
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
  std::string login)
{
  std::string result_str("User not found");

	CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
		[
      login,
			result,
      &result_str
		](database_read_transaction & t)->expected<void> {

		orm::transaction_log_record_dbo t1;
		GET_EXPECTED(st, t.get_reader(
			t1.select(t1.id, t1.data)
			.order_by(db_desc_order(t1.order_no))));

		WHILE_EXPECTED(st.execute())
			const auto data = t1.data.get(st);
			GET_EXPECTED(block, transactions::transaction_block::create(data));

			CHECK_EXPECTED(block.walk_messages(
        [login, &result, &result_str](const transactions::create_user_transaction & message)->expected<bool> {
          if (login == message.user_email) {
            auto res = std::make_shared<json_object>();
            CHECK_EXPECTED(res->add_property("public_key", message.user_public_key->str()));
            res->add_property("user_profile_id", base64::from_bytes(message.user_profile_id));

            result->add_property("result", res);

            return false;
          }

          return true;
      }));


			WHILE_EXPECTED_END()

			return expected<void>();
	}));

  if (!result->get_property("result")) {
    result->add_property("error", result_str);
  }

	co_return expected<void>();
}

vds::async_task<vds::expected<void>>
vds::websocket_api::upload(const vds::service_provider * sp, std::shared_ptr<json_object> result, const_data_buffer body)
{
  GET_EXPECTED_ASYNC(body_hash, hash::signature(hash::sha256(), body));
  GET_EXPECTED_ASYNC(info, co_await server_api(sp).upload_data(body));

  auto res = std::make_shared<json_object>();
     
  auto replicas = std::make_shared<json_array>();
  for (const auto & p : info) {
    replicas->add(std::make_shared<json_primitive>(base64::from_bytes(p)));
  }
  res->add_property("replicas", replicas);
  res->add_property("hash", base64::from_bytes(body_hash));

  result->add_property("result", res);

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
  auto result_json = std::make_shared<json_array>();
  
  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction([sp, result_json, owner_id](database_read_transaction & t) -> expected<void> {
    auto client = sp->get<dht::network::client>();
  
    orm::node_storage_dbo t1;
    orm::local_data_dbo t2;
    db_value<int64_t> used_size;
    GET_EXPECTED(st, t.get_reader(
      t1.select(
        t1.storage_id,
        t1.local_path,
        t1.reserved_size,
        db_sum(t2.replica_size).as(used_size))
      .inner_join(t2, t2.storage_id == t1.storage_id)
      .where(t1.owner_id == owner_id)
      .group_by(t1.local_path, t1.reserved_size)));
  
    WHILE_EXPECTED(st.execute()) {
      if (!t1.local_path.get(st).empty()) {
        auto result_item = std::make_shared<json_object>();
        result_item->add_property("id", base64::from_bytes(t1.storage_id.get(st)));
        result_item->add_property("local_path", t1.local_path.get(st));
        result_item->add_property("reserved_size", t1.reserved_size.get(st));
        result_item->add_property("used_size", used_size.get(st));

        auto free_size_result = foldername(t1.local_path.get(st)).free_size();
        if (free_size_result.has_value()) {
          result_item->add_property("free_size", free_size_result.value());
        }
        result_json->add(result_item);
      }
    }
    WHILE_EXPECTED_END()
    return expected<void>();
  }));

  res->add_property("result", result_json);
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::websocket_api::get_channel_messages(
  const vds::service_provider * sp,
  std::shared_ptr<json_object> result,
  const_data_buffer channel_id,
  int64_t last_id,
  uint32_t limit)
{
  auto items = std::make_shared<json_array>();

  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
    [
      sp,
      this_ = this->weak_from_this(),
      channel_id,
      items,
      last_id,
      limit
    ](database_read_transaction & t)->expected<void> {

    auto pthis = this_.lock();
    if (!pthis) {
      return expected<void>();
    }

    orm::channel_message_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.block_id, t1.channel_id, t1.read_id, t1.write_id, t1.crypted_key, t1.crypted_data, t1.signature)
      .where(t1.channel_id == channel_id && t1.id < last_id).order_by(db_desc_order(t1.id))));
    WHILE_EXPECTED(st.execute())
    {
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

      if (limit < items->size()) {
        break;
      }
    }
    WHILE_EXPECTED_END()

      return expected<void>();
  }));

  result->add_property("result", items);
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::websocket_api::allocate_storage(
  const vds::service_provider* sp,
  std::shared_ptr<json_object> result,
  asymmetric_public_key user_public_key,
  foldername folder,
  long size)
{
  GET_EXPECTED(json, json_parser::parse(".vds_storage.json", file::read_all(filename(folder, ".vds_storage.json"))));
  auto sign_info = std::dynamic_pointer_cast<json_object>(json);
  if (!sign_info) {
    return vds::make_unexpected<std::runtime_error>("Invalid format");
  }

  std::string version;
  if (!sign_info->get_property("vds", version) || version != "0.1") {
    return vds::make_unexpected<std::runtime_error>("Invalid file version");
  }

  GET_EXPECTED(key_id, user_public_key.fingerprint());
  const_data_buffer value;
  if (!sign_info->get_property("name", value) || value != key_id) {
    return vds::make_unexpected<std::runtime_error>("Invalid user name");
  }

  if (!sign_info->get_property("sign", value)) {
    return vds::make_unexpected<std::runtime_error>("The signature is missing");
  }

  auto sig_body = std::make_shared<json_object>();
  sig_body->add_property("vds", "0.1");
  sig_body->add_property("name", key_id);

  GET_EXPECTED(body, sig_body->json_value::str());
  GET_EXPECTED(sig_ok, asymmetric_sign_verify::verify(
    hash::sha256(),
    user_public_key,
    value,
    body.c_str(),
    body.length()));

  if (!sig_ok) {
    return vds::make_unexpected<std::runtime_error>("Invalid signature");
  }


  return sp->get<db_model>()->async_transaction([sp, key_id, folder, size, result](database_transaction& t) -> expected<void> {
    auto client = sp->get<dht::network::client>();

    binary_serializer s;
    CHECK_EXPECTED(s << client->current_node_id());
    CHECK_EXPECTED(s << key_id);
    CHECK_EXPECTED(s << folder.full_name());
    
    GET_EXPECTED(storage_id, hash::signature(hash::sha256(), s.move_data()));

    result->add_property("result", base64::from_bytes(storage_id));

    orm::node_storage_dbo t1;
    return t.execute(
      t1.insert(
        t1.storage_id = storage_id,
        t1.local_path = folder.full_name(),
        t1.owner_id = key_id,
        t1.reserved_size = (int64_t)size * 1024 * 1024));
    });
}

vds::async_task<vds::expected<void>> vds::websocket_api::download(
  const vds::service_provider * sp,
  std::shared_ptr<json_object> result,
  std::vector<const_data_buffer> object_ids)
{
  auto network_client = sp->get<dht::network::client>();
  GET_EXPECTED_ASYNC(buffer, co_await network_client->restore(object_ids));

  result->add_property("result", buffer);
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::websocket_api::broadcast(
  const vds::service_provider* sp,
  std::shared_ptr<json_object> result,
  const_data_buffer body)
{
  GET_EXPECTED_ASYNC(trx_id, co_await server_api(sp).broadcast(body));
  result->add_property("result", trx_id);

  co_return expected<void>();
}

vds::websocket_api::subscribe_handler::subscribe_handler(std::string cb, const_data_buffer channel_id)
  : cb_(std::move(cb)), channel_id_(std::move(channel_id)), last_id_(0)
{
}

vds::async_task<vds::expected<void>> vds::websocket_api::subscribe_handler::process(
  const vds::service_provider * sp,
  std::weak_ptr<websocket_output> output_stream)
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