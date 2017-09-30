#ifndef __VDS_PROTOCOLS_CHUNK_MANAGER_H_
#define __VDS_PROTOCOLS_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "local_cache.h"
#include "log_records.h"
#include "filename.h"

namespace vds {
  class _chunk_manager;
  class ichunk_manager
  {
  public:
    typedef uint64_t index_type;
    typedef uint16_t replica_type;

    struct chunk_store
    {
      guid server_id;
      index_type index;
      replica_type replica;
      guid storage_id;
    };


    ichunk_manager();
    ~ichunk_manager();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);
    
    async_task<> add_object(
      const service_provider & sp,
      database_transaction & tr,
      const guid & version_id,
      const filename & tmp_file,
      const const_data_buffer & file_hash);
    
    void get_object_map(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_id,
      guid & server_id,
      ichunk_manager::index_type & min_chunk_index,
      ichunk_manager::index_type & max_chunk_index);

    void query_object_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      index_type chunk_index,
      const guid & object_id,
      size_t & downloaded_data,
      size_t & total_data);

    void dump_state(
      const service_provider & sp,
      database_transaction & tr,
      const std::shared_ptr<json_object> & result);
    
    _chunk_manager * operator -> ();
  };
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_H_
