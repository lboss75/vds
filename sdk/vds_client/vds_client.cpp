/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_client.h"
#include "json_object.h"
#include "transaction_block_builder.h"

vds::async_task<vds::expected<std::vector<vds::const_data_buffer>>> vds::vds_client::upload_data(const const_data_buffer& data)
{
  auto args = std::make_shared<json_array>();
  args->add(std::make_shared<json_primitive>(base64::from_bytes(data)));

  GET_EXPECTED_ASYNC(res, co_await this->invoke("upload", args));
  auto res_obj = std::dynamic_pointer_cast<json_object>(res);

  auto replicas_props = res_obj->get_property("replicas");
  auto replicas = std::dynamic_pointer_cast<json_array>(replicas_props);
  auto count = replicas->size();

  std::vector<const_data_buffer> result;
  for (int i = 0; i < count; ++i) {
    auto value = std::dynamic_pointer_cast<json_primitive>(replicas->get(i));
    if (!value) {
      co_return make_unexpected<std::runtime_error>("Invalid result of upload operation");
    }
    GET_EXPECTED_ASYNC(item, base64::to_bytes(value->value()));
    result.push_back(item);
  }

  return result;
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::vds_client::broadcast(transactions::transaction_block_builder& builder)
{
  const auto data = builder.close();

  auto args = std::make_shared<json_array>();
  args->add(std::make_shared<json_primitive>(base64::from_bytes(data)));

  return this->invoke("broadcast", args);
}

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::vds_client::invoke(const std::string& method, std::shared_ptr<json_array> args)
{
  const auto id = ++this->last_id_;
  auto obj = std::make_shared<json_object>();
  obj->add_property("id", id);
  obj->add_property("invoke", method);
  obj->add_property("params", args);

  auto& r = this->callbacks_[id];
  GET_EXPECTED_ASYNC(message, obj->json_value::str());
  CHECK_EXPECTED_ASYNC(co_await this->ws_->send_message(message));

  co_return co_await r.get_future();
}
