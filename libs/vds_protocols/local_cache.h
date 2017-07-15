#ifndef __VDS_PROTOCOLS_LOCAL_CACHE_H_
#define __VDS_PROTOCOLS_LOCAL_CACHE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"
#include "server_task_manager.h"

namespace vds {
  class _local_cache;
  class full_storage_object_id;
  
  class ilocal_cache
  {
  public:
    std::unique_ptr<const_data_buffer> get_object(
      const service_provider & sp,
      const full_storage_object_id & object_id);

    filename get_object_filename(
      const service_provider & sp,
      const guid & server_id,
      uint64_t index);
    
    async_task<server_task_manager::task_state> download_object(
      const service_provider & sp,
      const guid & version_id);

  };
}

#endif // __VDS_PROTOCOLS_LOCAL_CACHE_H_
