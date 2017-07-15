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

    struct object_chunk_map
    {
      guid server_id;
      index_type chunk_index;
      guid object_id;
      size_t object_offset;
      size_t chunk_offset;
      size_t length;
      const_data_buffer hash;
    };

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
      std::list<object_chunk_map> & result);

    void query_object_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      index_type chunk_index,
      size_t & downloaded_data,
      size_t & total_data);
    
    _chunk_manager * operator -> ();
  };
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_H_
