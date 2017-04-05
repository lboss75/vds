#ifndef __VDS_STORAGE_CHUNK_MANAGER_H_
#define __VDS_STORAGE_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"
#include "log_records.h"

namespace vds {
  class _chunk_manager;
  
  class chunk_manager
  {
  public:
    chunk_manager(
      const service_provider & sp,
      const guid & server_id,
      const asymmetric_private_key & private_key,
      local_cache & cache);
    ~chunk_manager();


    class file_map
    {
    public:
      void add(const server_log_new_object & item);

    private:
      std::list<server_log_new_object> items_;
    };


    async_task<const file_map &> add(const filename & fn);
    
    async_task<const server_log_new_object &> add(const data_buffer & data);
    
    void set_next_index(uint64_t next_index);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
