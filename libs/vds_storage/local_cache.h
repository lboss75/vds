#ifndef __VDS_STORAGE_LOCAL_CACHE_H_
#define __VDS_STORAGE_LOCAL_CACHE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"

namespace vds {
  class _local_cache;
  
  class local_cache
  {
  public:
    local_cache(const service_provider & sp);
    ~local_cache();
    
    std::unique_ptr<data_buffer> get_object(
      const full_storage_object_id& object_id);
    
    filename get_object_filename(
      const guid & server_id,
      uint64_t index);
  
  private:
    _local_cache * impl_;
  };
}

#endif // __VDS_STORAGE_LOCAL_CACHE_H_
