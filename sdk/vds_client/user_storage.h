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
      const_data_buffer owner_id;
		  const_data_buffer node_id;
		  foldername local_path;
		  uint64_t reserved_size;
		  uint64_t used_size;
		  uint64_t free_size;

		  std::shared_ptr<json_value> serialize() const;
	  };

    static async_task<expected<storage_info_t>>
      device_storage(const service_provider * sp);

    static expected<std::shared_ptr<json_value>>
      device_storage_label(
        const std::shared_ptr<user_manager> &user_mng);

    static async_task<expected<void>>
      set_device_storage(
        const service_provider * sp,
        const std::shared_ptr<user_manager> & user_mng,
        const std::string &local_path,
        uint64_t reserved_size);
  };
}

#endif // __VDS_USER_MANAGER_USER_STORAGE_H_
