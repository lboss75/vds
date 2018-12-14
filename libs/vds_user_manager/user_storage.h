#ifndef __VDS_USER_MANAGER_USER_STORAGE_H_
#define __VDS_USER_MANAGER_USER_STORAGE_H_

#include "json_object.h"
#include "user_manager.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class user_storage {
  public:
    static async_task<std::shared_ptr<json_value>>
      device_storages(
        const service_provider * sp,
        const std::shared_ptr<user_manager> & user_mng);

    static std::shared_ptr<json_value>
      device_storage_label(
        const std::shared_ptr<user_manager> &user_mng);

    static async_task<void>
      add_device_storage(
        const service_provider * sp,
        const std::shared_ptr<user_manager> & user_mng,
        const std::string &name,
        const std::string &local_path,
        uint64_t reserved_size);

    static async_task<bool>
      local_storage_exists(
        const service_provider * sp);
  };
}

#endif // __VDS_USER_MANAGER_USER_STORAGE_H_
