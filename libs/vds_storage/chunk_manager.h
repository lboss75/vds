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

    class file_map
    {
    public:
      struct item
      {
        uint64_t index_;

        item(uint64_t index)
          : index_(index)
        {
        }
      };

      void push_back(const item & v);

    private:
      std::list<item> items_;
    };

    std::future<file_map> add(const filename & fn);

    struct object_index
    {
      uint64_t index;
      data_buffer signature;
    };

    void add(
      const std::function<void (chunk_manager::file_map) > & done,
      const error_handler & on_error,
      const filename & fn);
    
    void add(
      const std::function<void (chunk_manager::object_index) > & done,
      const error_handler & on_error,
      const data_buffer& data);
    
    void set_next_index(uint64_t next_index);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
