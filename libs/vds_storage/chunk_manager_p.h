#ifndef __VDS_STORAGE_CHUNK_MANAGER_P_H_
#define __VDS_STORAGE_CHUNK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"

namespace vds {
  class _chunk_manager
  {
  public:
    _chunk_manager(
      const service_provider & sp,
      const guid & server_id,
      local_cache & cache,
      chunk_manager * owner);
    ~_chunk_manager();

    async_task<const chunk_manager::file_map &>
      add(const filename & fn);
    
    async_task<const chunk_manager::object_index &>
      add(const data_buffer& data);
    
    void set_next_index(uint64_t next_index);

  private:
    chunk_manager * owner_;
    guid server_id_;
    local_cache & cache_;

    std::mutex file_mutex_;
    uint64_t last_index_;
    uint64_t last_chunk_;
    file output_file_;
    
    foldername tmp_folder_;
    std::mutex tmp_folder_mutex_;
    uint64_t last_tmp_file_index_;
    
    std::mutex obj_folder_mutex_;
    uint64_t last_obj_file_index_;
    uint64_t obj_size_;
    
    static constexpr uint64_t  max_obj_size_ = 1024 * 1024 * 1024;

    enum chunk_block_type
    {
      cbt_start_stream = 1,
      cbt_add_stream = 2,
      cbt_finish_stream = 3
    };

    static constexpr size_t output_file_max_size = 1024;

    void generate_chunk();
  };

}

#endif // __VDS_STORAGE_CHUNK_MANAGER_P_H_
