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
	  struct storage_info_t {
		  const_data_buffer node_id;
		  std::string name;
		  foldername local_path;
		  uint64_t reserved_size;
		  uint64_t used_size;
		  uint64_t free_size;
		  bool current;

		  std::shared_ptr<json_value> serialize() const;
	  };

    static async_task<expected<std::list<storage_info_t>>>
      device_storages(
        const service_provider * sp,
        const_data_buffer owner_id);

    static expected<std::shared_ptr<json_value>>
      device_storage_label(
        const std::shared_ptr<user_manager> &user_mng);

    static async_task<expected<void>>
      add_device_storage(
        const service_provider * sp,
        const std::shared_ptr<user_manager> & user_mng,
        const std::string &name,
        const std::string &local_path,
        uint64_t reserved_size);

	static async_task<expected<void>>
		set_device_storage(
			const service_provider * sp,
			const std::shared_ptr<user_manager> & user_mng,
			const std::string &local_path,
			uint64_t reserved_size);

    static async_task<expected<bool>>
      local_storage_exists(
        const service_provider * sp);
  };
}

#endif // __VDS_USER_MANAGER_USER_STORAGE_H_
