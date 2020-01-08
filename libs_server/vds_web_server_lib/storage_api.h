//#ifndef __VDS_WEB_SERVER_LIB_STORAGE_API_H_
//#define __VDS_WEB_SERVER_LIB_STORAGE_API_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "user_manager.h"
//
//namespace vds {
//  class http_message;
//
//  class storage_api {
//  public:
//
//    static vds::async_task<vds::expected<std::shared_ptr<vds::json_value>>>
//    device_storages(
//        const vds::service_provider * sp,
//        const std::shared_ptr<user_manager> & user_mng,
//        const http_message & request);
//
//    static async_task<expected<std::shared_ptr<vds::json_value>>>
//    device_storage_label(
//      const vds::service_provider * sp,
//      const std::shared_ptr<user_manager> &user_mng,
//      const http_message & request);
//
//    static async_task<vds::expected<void>>
//    add_device_storage(
//      const vds::service_provider * sp,
//      const std::shared_ptr<user_manager> & user_mng,
//      const std::string &local_path,
//      uint64_t reserved_size);
//  };
//
//}//vds
//
//#endif //__VDS_WEB_SERVER_LIB_STORAGE_API_H_
