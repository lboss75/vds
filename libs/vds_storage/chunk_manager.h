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
    chunk_manager(const service_provider & sp);
    ~chunk_manager();

    void start();
    void stop();

  private:
    friend class ichunk_manager;
    _chunk_manager * const impl_;
  };

  class ichunk_manager
  {
  public:
    ichunk_manager(chunk_manager * owner);

    async_task<const server_log_file_map &> add(
      const std::string & user_login,
      const std::string & name,
      const filename & fn);
    async_task<const server_log_new_object &> add(const data_buffer & data);
    void set_next_index(uint64_t next_index);

  private:
    chunk_manager * const owner_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
