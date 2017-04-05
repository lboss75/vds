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
      const asymmetric_private_key & private_key,
      local_cache & cache);
    ~chunk_manager();

    struct object_index
    {
    public:
      object_index(
        uint64_t index,
        uint32_t original_lenght,
        const data_buffer & original_hash,
        uint32_t target_lenght,
        const data_buffer & target_hash);
      
      uint64_t index() const { return this->index_; }
      uint32_t original_lenght() const { return this->original_lenght_; }
      const data_buffer & original_hash() const { return this->original_hash_; }
      uint32_t target_lenght() const { return this->target_lenght_; }
      const data_buffer & target_hash() const { return this->target_hash_; }
      
    private:
      uint64_t index_;
      uint32_t original_lenght_;
      data_buffer original_hash_;
      uint32_t target_lenght_;
      data_buffer target_hash_;
    };

    class file_map
    {
    public:
      void add(const object_index & item);

    private:
      std::list<object_index> items_;
    };


    async_task<const file_map &> add(const filename & fn);
    
    async_task<const object_index &> add(const data_buffer & data);
    
    void set_next_index(uint64_t next_index);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
