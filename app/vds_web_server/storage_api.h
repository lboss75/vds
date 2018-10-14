#ifndef __VDS_WEB_SERVER_STORAGE_API_H_
#define __VDS_WEB_SERVER_STORAGE_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "user_manager.h"

namespace vds {
  class http_request;

  class storage_api {
  public:

    static vds::async_task<std::shared_ptr<vds::json_value>>
    device_storages(
        const vds::service_provider * sp,
      const std::shared_ptr<user_manager> & user_mng,
        const http_request & request);

    static std::shared_ptr<vds::json_value>
    device_storage_label(
        const std::shared_ptr<user_manager> &user_mng);

    static vds::async_task<void>
    add_device_storage(
      const vds::service_provider * sp,
      const std::shared_ptr<user_manager> & user_mng,
      const std::string &name,
      const std::string &local_path,
      uint64_t reserved_size);

  };

}//vds

#endif //__VDS_WEB_SERVER_STORAGE_API_H_
