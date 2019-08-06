/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_storage.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "member_user.h"
#include "current_config_dbo.h"

vds::async_task<vds::expected<vds::user_storage::storage_info_t>> vds::user_storage::device_storage(
  const service_provider* sp) {

	vds::user_storage::storage_info_t result;

  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction([sp, &result](database_read_transaction & t) -> expected<void> {
    auto client = sp->get<dht::network::client>();

    orm::current_config_dbo t1;
    orm::device_record_dbo t2;
    db_value<int64_t> used_size;
    GET_EXPECTED(st, t.get_reader(
      t1.select(
        t1.node_id,
        t1.owner_id,
        t1.local_path,
        t1.reserved_size,
        db_sum(t2.data_size).as(used_size))
      .left_join(t2, t2.node_id == t1.node_id)
      .group_by(t1.node_id, t1.owner_id, t1.local_path, t1.reserved_size)));

    WHILE_EXPECTED(st.execute()) {
      if (!t1.local_path.get(st).empty()) {
        result.node_id = t1.node_id.get(st);
        result.owner_id = t1.owner_id.get(st);
        result.local_path = foldername(t1.local_path.get(st));
        result.reserved_size = safe_cast<uint64_t>(t1.reserved_size.get(st));
        result.used_size = safe_cast<uint64_t>(used_size.get(st));

        auto free_size_result = foldername(result.local_path).free_size();
        if (free_size_result.has_value()) {
          result.free_size = free_size_result.value();
        }
      }
    }
    WHILE_EXPECTED_END()
    return expected<void>();
  }));

  co_return result;
}

vds::expected<std::shared_ptr<vds::json_value>> vds::user_storage::device_storage_label(
  const std::shared_ptr<user_manager>& user_mng) {
  auto result = std::make_shared<json_object>();

  auto user = user_mng->get_current_user();
  GET_EXPECTED(key_id, user.user_public_key()->fingerprint());
  result->add_property("vds", "0.1");
  result->add_property("name", key_id);

  GET_EXPECTED(body, result->json_value::str());
  GET_EXPECTED(sig, asymmetric_sign::signature(
    hash::sha256(),
    *user.private_key(),
    body.c_str(),
    body.length()));

  result->add_property(
    "sign",
    base64::from_bytes(sig));

  return std::static_pointer_cast<json_value>(result);
}

vds::async_task<vds::expected<void>> vds::user_storage::set_device_storage(
  const service_provider* sp,
  const std::shared_ptr<user_manager>& user_mng,
  const std::string& local_path,
  uint64_t reserved_size) {
  GET_EXPECTED(json, json_parser::parse(".vds_storage.json", file::read_all(filename(foldername(local_path), ".vds_storage.json"))));
  auto sign_info = std::dynamic_pointer_cast<json_object>(json);
  if (!sign_info) {
    return vds::make_unexpected<std::runtime_error>("Invalid format");
  }

  std::string version;
  if (!sign_info->get_property("vds", version) || version != "0.1") {
    return vds::make_unexpected<std::runtime_error>("Invalid file version");
  }

  auto user = user_mng->get_current_user();
  GET_EXPECTED(key_id, user.user_public_key()->fingerprint());
  const_data_buffer value;
  if (!sign_info->get_property("name", value) || value != key_id) {
    return vds::make_unexpected<std::runtime_error>("Invalid user name");
  }

  auto result = std::make_shared<json_object>();
  result->add_property("vds", "0.1");
  result->add_property("name", key_id);

  GET_EXPECTED(body, result->json_value::str());
  GET_EXPECTED(sig, asymmetric_sign::signature(
    hash::sha256(),
    *user.private_key(),
    body.c_str(),
    body.length()));

  if (!sign_info->get_property("sign", value) || value != sig) {
    return vds::make_unexpected<std::runtime_error>("Invalid signature");
  }

  return sp->get<db_model>()->async_transaction([sp, user_mng, local_path, reserved_size](database_transaction & t) -> expected<void> {
    auto user = user_mng->get_current_user();
    auto client = sp->get<dht::network::client>();
    auto current_node = client->current_node_id();

    GET_EXPECTED(owner_id, user.user_public_key()->fingerprint());

    orm::current_config_dbo t1;
    return t.execute(
      t1.update(
        t1.local_path = local_path,
        t1.owner_id = owner_id,
        t1.reserved_size = reserved_size)
    .where(t1.node_id == client->current_node_id()));
  });
}


std::shared_ptr<vds::json_value> vds::user_storage::storage_info_t::serialize() const
{
	auto item = std::make_shared<json_object>();
	item->add_property("node", base64::from_bytes(this->node_id));
	item->add_property("local_path", this->local_path.full_name());
	item->add_property("reserved_size", this->reserved_size);
	item->add_property("used_size", std::to_string(this->used_size));
	item->add_property("free_size", std::to_string(this->free_size));

	return item;
}
