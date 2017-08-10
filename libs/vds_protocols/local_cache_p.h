#ifndef __VDS_PROTOCOLS_LOCAL_CACHE_P_H_
#define __VDS_PROTOCOLS_LOCAL_CACHE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"

namespace vds {
  class _local_cache : public ilocal_cache
  {
  public:
    _local_cache();
    ~_local_cache();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);
    
   
    filename get_object_filename(
      const service_provider & sp,
      const guid & server_id,
      uint64_t index);
    
    async_task<server_task_manager::task_state> download_object(
      const service_provider & sp,
      const guid & version_id,
      const filename & result_data);

  private:
    foldername root_folder_;
  };
}

#endif // __VDS_PROTOCOLS_LOCAL_CACHE_P_H_
