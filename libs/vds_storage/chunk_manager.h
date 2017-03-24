#ifndef __VDS_STORAGE_CHUNK_MANAGER_H_
#define __VDS_STORAGE_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"

namespace vds {
  class _chunk_manager;
  
  class chunk_manager
  {
  public:
    chunk_manager(
      const service_provider & sp,
      const guid & server_id,
      local_cache & cache);
    ~chunk_manager();

    void add(const filename & fn, std::list<uint64_t> & parts);
    uint64_t add(const data_buffer & data);
    
    void set_next_index(uint64_t next_index);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
