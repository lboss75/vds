#ifndef __VDS_PROTOCOLS_LOCAL_CACHE_P_H_
#define __VDS_PROTOCOLS_LOCAL_CACHE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"

namespace vds {
  class _local_cache
  {
  public:
    _local_cache(
      const service_provider & sp,
      local_cache * owner);
    ~_local_cache();
    
    std::unique_ptr<const_data_buffer> get_object(
      const full_storage_object_id& object_id);
    
    filename get_object_filename(
      const guid & server_id,
      uint64_t index);
  
  private:
    service_provider sp_;
    local_cache * owner_;
    foldername root_folder_;
  };
}

#endif // __VDS_PROTOCOLS_LOCAL_CACHE_P_H_
