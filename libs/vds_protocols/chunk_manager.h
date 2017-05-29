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
  class _chunk_manager;
  
  class ichunk_manager
  {
  public:
    async_task<> add(
      const service_provider & sp,
      server_log_file_map & target,
      const filename & fn);

    const_data_buffer get(
      const service_provider & sp,
      const guid & server_id,
      uint64_t index);
    
    void set_next_index(
      const service_provider & sp,
      uint64_t next_index);
  };
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_H_
