#ifndef __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
#define __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "server_database.h"
#include "chunk_storage.h"
#include "database_orm.h"
#include "task_manager.h"

namespace vds {
  class istorage_log;

  class _chunk_manager : public ichunk_manager
  {
  public:
    _chunk_manager();
    ~_chunk_manager();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

    async_task<> add_object(
      const service_provider & sp,
      database_transaction & tr,
      const guid & version_id,
      const filename & tmp_file,
      const const_data_buffer & file_hash);

    static void create_database_objects(
      const service_provider & sp,
      uint64_t db_version,
      database_transaction & t);

    void get_object_map(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_id,
      guid & server_id,
      ichunk_manager::index_type & min_chunk_index,
      ichunk_manager::index_type & max_chunk_index);

    void get_replicas(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      ichunk_manager::index_type index,
      const guid & storage_id,
      std::list<ichunk_manager::replica_type> & result);

    const_data_buffer get_replica_data(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      ichunk_manager::index_type index,
      ichunk_manager::replica_type replica);

    void get_chunk_store(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      index_type index,
      std::list<chunk_store> & result);

    void add_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      ichunk_manager::index_type chunk_index,
      const guid & object_id,
      size_t chunk_size,
      const const_data_buffer & chunk_hash);
    
    void add_chunk_replica(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      ichunk_manager::replica_type replica,
      size_t replica_length,
      const const_data_buffer & replica_hash);
    
    void query_object_chunk(
      const service_provider& sp,
      database_transaction& tr,
      const guid & server_id,
      index_type chunk_index,
      const guid & object_id,
      size_t & downloaded_data,
      size_t& total_data);
    
    const_data_buffer restore_object_chunk(
      const vds::service_provider& sp,
      vds::database_transaction& tr,
      const vds::guid & server_id,
      vds::ichunk_manager::index_type chunk_index,
      const guid & object_id,
      size_t & chunk_size,
      const_data_buffer & chunk_hash);

    void add_chunk_store_data(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      uint16_t replica,
      const guid & storage_id,
      const const_data_buffer & data);

    void dump_state(
      const service_provider & sp,
      database_transaction & tr,
      const std::shared_ptr<json_object> & result);

    static const size_t BLOCK_SIZE = 16 * 1024 * 1024;
    static const uint16_t MIN_HORCRUX = 512;
    static const uint16_t GENERATE_HORCRUX = 1024;
    
  private:
    friend class _server_log_logic;
    
    chunk_storage chunk_storage_;

    std::mutex chunk_mutex_;
    uint64_t last_chunk_;
    
    timer update_chunk_map_;
    
    //Database
    class object_chunk_table : public database_table
    {
    public:
      object_chunk_table()
        : database_table("object_chunk"),
          server_id(this, "server_id"),
          chunk_index(this, "chunk_index"),
          object_id(this, "object_id"),
          chunk_size(this, "chunk_size"),
          chunk_hash(this, "chunk_hash")
      {}

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<guid> object_id;
      database_column<size_t> chunk_size;
      database_column<const_data_buffer> chunk_hash;
    };

    class object_chunk_replica_table : public database_table
    {
    public:
      object_chunk_replica_table()
        : database_table("object_chunk_replica"),
        server_id(this, "server_id"),
        chunk_index(this, "chunk_index"),
        replica(this, "replica"),
        replica_length(this, "replica_length"),
        replica_hash(this, "replica_hash")
      {
      }

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<int> replica;
      database_column<size_t> replica_length;
      database_column<const_data_buffer> replica_hash;
    };

    class object_chunk_store_table : public database_table
    {
    public:
      object_chunk_store_table()
        : database_table("object_chunk_store"),
          replica(this, "replica"),
          server_id(this, "server_id"),
          chunk_index(this, "chunk_index"),
          storage_id(this, "storage_id")
      {
      }

      database_column<int> replica;
      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<guid> storage_id;
    };

    class object_chunk_data_table : public database_table
    {
    public:
      object_chunk_data_table()
        : database_table("object_chunk_data"),
        server_id(this, "server_id"),
        chunk_index(this, "chunk_index"),
        replica(this, "replica"),
        data(this, "data")
      {
      }

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<int> replica;
      database_column<const_data_buffer> data;
    };

    bool write_chunk(
      const service_provider & sp,
      database_transaction & tr,
      size_t chunk_index,
      const guid & object_id,
      const filename & tmp_file,
      size_t offset,
      size_t size,
      const error_handler & on_error,
      bool is_last);

    bool generate_horcruxes(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      principal_log_new_chunk & chunk_info,
      const std::vector<uint8_t> & buffer,
      const error_handler & on_error);

    bool generate_tail_horcruxes(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t chunk_index,
      const error_handler & on_error);

    size_t get_last_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id);
    
    void add_chunk_store(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      uint16_t replica,
      const guid & storage_id);
    
    void update_chunk_map(
      const service_provider & sp,
      database_transaction & tr);

    void update_move_replicas(
      const service_provider & sp,
      database_transaction & tr);
    
    void dump_local_chunks(
      const service_provider & sp,
      database_transaction & tr,
      const std::shared_ptr<json_object>& result);

    void dump_chunks_map(
      const service_provider & sp,
      database_transaction & tr,
      const std::shared_ptr<json_object>& result);
  };

  inline _chunk_manager * vds::ichunk_manager::operator->()
  {
    return static_cast<_chunk_manager *>(this);
  }
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
