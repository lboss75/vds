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

    struct object_index
    {
      uint64_t index;

      uint32_t original_lenght;
      data_buffer original_hash;

      uint32_t target_lenght;
      data_buffer signature;
    };

    class file_map
    {
    public:
      void add(const object_index & item);

    private:
      std::list<object_index> items_;
    };


    async_task<void(const file_map &)> add(const filename & fn);
    
    async_task<void(const object_index &)> add(const data_buffer & data);
    
    void set_next_index(uint64_t next_index);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
