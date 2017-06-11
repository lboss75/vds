#ifndef __VDS_PROTOCOLS_CHUNK_MANAGER_H_
#define __VDS_PROTOCOLS_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"
#include "log_records.h"
#include "filename.h"

namespace vds {
  class ichunk_manager
  {
  public:
    ichunk_manager();
    ~ichunk_manager();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);
    
    async_task<> add_object(
      const service_provider & sp,
      const guid & version_id,
      const filename & tmp_file);
  };
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_H_
