#ifndef __VDS_STORAGE_CHUNK_MANAGER_P_H_
#define __VDS_STORAGE_CHUNK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "server_database.h"

namespace vds {
  class istorage_log;

  class _chunk_manager
  {
  public:
    _chunk_manager(
      const service_provider & sp,
      chunk_manager * owner);
    ~_chunk_manager();

    async_task<const server_log_file_map &>
      add(
        const std::string & version_id,
        const std::string & user_login,
        const std::string & name,
        const filename & fn);
    
    async_task<const server_log_new_object &>
      add(const data_buffer& data);
    
    void set_next_index(uint64_t next_index);

    void start();
    void stop();

  private:
    service_provider sp_;
    chunk_manager * owner_;

    std::mutex file_mutex_;
    uint64_t last_index_;
    uint64_t last_chunk_;
    
    std::mutex tmp_folder_mutex_;
    foldername tmp_folder_;
    uint64_t last_tmp_file_index_;
    
    std::mutex obj_folder_mutex_;
    uint64_t last_obj_file_index_;
    uint64_t obj_size_;

    lazy_service<iserver_database> db_;
    lazy_service<istorage_log> storage_log_;
    lazy_service<ilocal_cache> cache_;

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
