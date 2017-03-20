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
      chunk_manager * owner);
    ~_chunk_manager();

    uint64_t start_stream();
    void add_stream(uint64_t id, const void * data, size_t len);
    void finish_stream(uint64_t id);

  private:
    chunk_manager * owner_;

    std::mutex file_mutex_;
    uint64_t last_index_;
    uint64_t last_chunk_;
    file output_file_;

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
