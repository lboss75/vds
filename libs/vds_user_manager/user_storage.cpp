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

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::user_storage::device_storages(
  const service_provider* sp,
  const std::shared_ptr<user_manager>& user_mng) {

  auto result = std::make_shared<json_array>();

  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction([sp, user_mng, result](database_read_transaction & t) -> expected<void> {
    auto client = sp->get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    orm::device_record_dbo t2;
    db_value<int64_t> used_size;
    GET_EXPECTED(st, t.get_reader(
      t1.select(
        t1.name,
        t1.node_id,
        t1.local_path,
        t1.reserved_size,
        db_sum(t2.data_size).as(used_size))
      .left_join(t2, t2.local_path == t1.local_path && t2.node_id == t1.node_id)
      .where(t1.owner_id == user_mng->get_current_user().user_certificate()->subject())
      .group_by(t1.name, t1.node_id, t1.local_path, t1.reserved_size)));
    WHILE_EXPECTED (st.execute())
      auto item = std::make_shared<json_object>();
      item->add_property("node", base64::from_bytes(t1.node_id.get(st)));
      item->add_property("name", t1.name.get(st));
      item->add_property("local_path", t1.local_path.get(st));
      item->add_property("reserved_size", t1.reserved_size.get(st));
      item->add_property("used_size", std::to_string(used_size.get(st)));
      if (t1.node_id.get(st) == current_node) {
        auto free_size = foldername(t1.local_path.get(st)).free_size();
        if (free_size.has_value()) {
          item->add_property("free_size", std::to_string(free_size.value()));
        }
        item->add_property("current", "true");
      }
      else {
        item->add_property("current", "false");
      }
      result->add(item);
    WHILE_EXPECTED_END()
    return expected<void>();
  }));

  co_return std::static_pointer_cast<json_value>(result);
}

vds::expected<std::shared_ptr<vds::json_value>> vds::user_storage::device_storage_label(
  const std::shared_ptr<user_manager>& user_mng) {
  auto result = std::make_shared<json_object>();

  auto user = user_mng->get_current_user();
  result->add_property("vds", "0.1");
  result->add_property("name", user.user_certificate()->subject());

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

vds::async_task<vds::expected<void>> vds::user_storage::add_device_storage(const service_provider* sp,
  const std::shared_ptr<user_manager>& user_mng, const std::string& name, const std::string& local_path,
  uint64_t reserved_size) {
  GET_EXPECTED(json, json_parser::parse("vds_storage.json", file::read_all(filename(foldername(local_path), "vds_storage.json"))));
  auto sign_info = std::dynamic_pointer_cast<json_object>(json);
  if (!sign_info) {
    return vds::make_unexpected<std::runtime_error>("Invalid format");
  }

  std::string version;
  if (!sign_info->get_property("vds", version) || version != "0.1") {
    return vds::make_unexpected<std::runtime_error>("Invalid file version");
  }

  auto user = user_mng->get_current_user();
  std::string value;
  if (!sign_info->get_property("name", value) || value != user.user_certificate()->subject()) {
    return vds::make_unexpected<std::runtime_error>("Invalid user name");
  }

  auto result = std::make_shared<json_object>();
  result->add_property("vds", "0.1");
  result->add_property("name", user.user_certificate()->subject());

  GET_EXPECTED(body, result->json_value::str());
  GET_EXPECTED(sig, asymmetric_sign::signature(
    hash::sha256(),
    *user.private_key(),
    body.c_str(),
    body.length()));

  if (!sign_info->get_property("sign", value) || value != base64::from_bytes(sig)) {
    return vds::make_unexpected<std::runtime_error>("Invalid signature");
  }

  return sp->get<db_model>()->async_transaction([sp, user_mng, local_path, name, reserved_size](database_transaction & t) -> expected<void> {
    auto user = user_mng->get_current_user();
    auto client = sp->get<dht::network::client>();
    auto current_node = client->current_node_id();

    GET_EXPECTED(storage_key_data, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
    auto storage_key = std::make_shared<asymmetric_private_key>(std::move(storage_key_data));

    GET_EXPECTED(public_key, asymmetric_public_key::create(*storage_key));

    certificate::create_options options;
    options.name = "!Storage Cert";
    options.country = "RU";
    options.organization = "IVySoft";
    options.ca_certificate = user.user_certificate().get();
    options.ca_certificate_private_key = user.private_key().get();

    GET_EXPECTED(storage_cert_data, certificate::create_new(public_key, *storage_key, options));
    auto storage_cert = std::make_shared<certificate>(std::move(storage_cert_data));
    GET_EXPECTED(storage_cert_der, storage_cert->der());
    GET_EXPECTED(storage_cert_key_der, storage_key->der(std::string()));

    orm::device_config_dbo t1;
    return t.execute(
      t1.insert(
        t1.node_id = client->current_node_id(),
        t1.local_path = local_path,
        t1.owner_id = user.user_certificate()->subject(),
        t1.name = name,
        t1.reserved_size = reserved_size,
        t1.cert = storage_cert_der,
        t1.private_key = storage_cert_key_der));
  });
}

vds::async_task<vds::expected<bool>> vds::user_storage::local_storage_exists(const service_provider* sp) {

  auto result = std::make_shared<bool>();

  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction([sp, result](database_read_transaction & t) -> expected<void> {
    auto client = sp->get<dht::network::client>();
    auto current_node = client->current_node_id();

    orm::device_config_dbo t1;
    db_value<int> storage_count;
    GET_EXPECTED(st, t.get_reader(
      t1.select(db_count(t1.name).as(storage_count))
      .where(t1.node_id == current_node && t1.owner_id != cert_control::get_storage_certificate()->subject())));

    auto st_result = st.execute();
    CHECK_EXPECTED_ERROR(st_result);

    if (st_result.value()) {
      *result = (storage_count.get(st) != 0);
    }
    else {
      *result = false;
    }
    return expected<void>();
  }));

  co_return *result;
}
