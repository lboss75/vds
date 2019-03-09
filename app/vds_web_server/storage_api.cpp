/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "storage_api.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"
#include "member_user.h"
#include "user_storage.h"

vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::storage_api::device_storages(
	const vds::service_provider * sp,
	const std::shared_ptr<user_manager> & user_mng,
	const http_request & /*request*/)
{
	auto result = co_await user_storage::device_storages(sp, user_mng);
	CHECK_EXPECTED_ASYNC(result);

	auto result_json = std::make_shared<json_array>();
	for (const auto & storage : result.value()) {
		result_json->add(storage.serialize());
	}

	co_return result_json;
}

vds::expected<std::shared_ptr<vds::json_value>> vds::storage_api::device_storage_label(
  const std::shared_ptr<user_manager>& user_mng)
{
  return user_storage::device_storage_label(user_mng);
}

vds::async_task<vds::expected<void>> vds::storage_api::add_device_storage(
  const vds::service_provider * sp,
  const std::shared_ptr<user_manager> & user_mng,
  const std::string & name,
  const std::string & local_path,
  uint64_t reserved_size){ 
  return user_storage::add_device_storage(sp, user_mng, name, local_path, reserved_size);
}
