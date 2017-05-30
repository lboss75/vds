#ifndef __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
#define __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "server_database.h"

namespace vds {
  class istorage_log;

  class _chunk_manager : public ichunk_manager
  {
  public:
    _chunk_manager();
    ~_chunk_manager();

    
    async_task<>
      add(
        const service_provider & sp,
        server_log_file_map & target,
        const filename & fn);

    void set_next_index(
      const service_provider & sp,
      uint64_t next_index);

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    std::mutex file_mutex_;
    uint64_t last_index_;
    uint64_t last_chunk_;
    
    std::mutex tmp_folder_mutex_;
    foldername tmp_folder_;
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

    void generate_chunk(const service_provider & sp);
  };

}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
