///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//#include "stdafx.h"
//#include "storage_api.h"
//#include "db_model.h"
//#include "dht_network_client.h"
//#include "device_config_dbo.h"
//#include "device_record_dbo.h"
//#include "member_user.h"
//#include "user_storage.h"
//#include "http_message.h"
//
//vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::storage_api::device_storages(
//	const vds::service_provider * sp,
//	const std::shared_ptr<user_manager> & user_mng,
//	const http_message & /*request*/)
//{
//  GET_EXPECTED_ASYNC(owner_id, user_mng->get_current_user().user_public_key()->fingerprint());
//
//	GET_EXPECTED_ASYNC(result, co_await user_storage::device_storage(sp));
//
//	auto result_json = result.serialize();
//	co_return result_json;
//}
//
//vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>> vds::storage_api::device_storage_label(
//  const vds::service_provider * sp,
//  const std::shared_ptr<user_manager>& user_mng,
//  const http_message & request)
//{
//  co_return user_storage::device_storage_label(user_mng);
//}
//
//vds::async_task<vds::expected<void>> vds::storage_api::add_device_storage(
//  const vds::service_provider * sp,
//  const std::shared_ptr<user_manager> & user_mng,
//  const std::string & local_path,
//  uint64_t reserved_size){ 
//  return user_storage::set_device_storage(sp, user_mng, local_path, reserved_size);
//}
