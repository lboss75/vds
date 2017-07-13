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

    std::list<object_chunk_map> get_object_map(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_id);

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

    void add_full_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_id,
      size_t offset,
      size_t size,
      const const_data_buffer & object_hash,
      const guid & server_id,
      size_t index);
    
    void add_chunk_replica(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      ichunk_manager::replica_type replica,
      size_t replica_length,
      const const_data_buffer & replica_hash);
    
    void add_object_chunk_map(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      ichunk_manager::index_type chunk_index,
      const guid & object_id,
      size_t object_offset,
      size_t chunk_offset,
      size_t length,
      const const_data_buffer & hash);
  private:
    chunk_storage chunk_storage_;

    std::mutex chunk_mutex_;
    uint64_t last_chunk_;
    
    std::mutex tail_chunk_mutex_;
    uint64_t tail_chunk_index_;
    size_t tail_chunk_size_;

    //Database
    class object_chunk_table : public database_table
    {
    public:
      object_chunk_table()
        : database_table("object_chunk"),
          server_id(this, "server_id"),
          chunk_index(this, "chunk_index"),
          chunk_size(this, "chunk_size"),
          hash(this, "hash")
      {}

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<size_t> chunk_size;
      database_column<const_data_buffer> hash;
    };

    class object_chunk_map_table : public database_table
    {
    public:
      object_chunk_map_table()
        : database_table("object_chunk_map"),
        server_id(this, "server_id"),
        chunk_index(this, "chunk_index"),
        object_id(this, "object_id"),
        object_offset(this, "object_offset"),
        chunk_offset(this, "chunk_offset"),
        length(this, "length"),
        hash(this, "hash")
      {}

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<guid> object_id;
      database_column<size_t> object_offset;
      database_column<size_t> chunk_offset;
      database_column<size_t> length;
      database_column<const_data_buffer> hash;
    };

    class tmp_object_chunk_table : public database_table
    {
    public:
      tmp_object_chunk_table()
        : database_table("tmp_object_chunk"),
        server_id(this, "server_id"),
        chunk_index(this, "chunk_index")
      {
      }

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
    };

    class tmp_object_chunk_map_table : public database_table
    {
    public:
      tmp_object_chunk_map_table()
        : database_table("tmp_object_chunk_map"),
        server_id(this, "server_id"),
        chunk_index(this, "chunk_index"),
        object_id(this, "object_id"),
        object_offset(this, "object_offset"),
        chunk_offset(this, "chunk_offset"),
        length(this, "length"),
        hash(this, "hash"),
        data(this, "data")
      {}

      database_column<guid> server_id;
      database_column<ichunk_manager::index_type> chunk_index;
      database_column<guid> object_id;
      database_column<size_t> object_offset;
      database_column<size_t> chunk_offset;
      database_column<size_t> length;
      database_column<const_data_buffer> hash;
      database_column<const_data_buffer> data;
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


    static const size_t BLOCK_SIZE = 16 * 1024 * 1024;
    static const uint16_t MIN_HORCRUX = 512;
    static const uint16_t GENERATE_HORCRUX = 1024;
    static const uint16_t REPLICA_SIZE = BLOCK_SIZE / MIN_HORCRUX;
    
    bool write_chunk(
      const service_provider & sp,
      database_transaction & tr,
      principal_log_new_object_map & result_record,
      const filename & tmp_file,
      size_t offset,
      size_t size,
      const error_handler & on_error);

    bool write_tail(
      const service_provider & sp,
      database_transaction & tr,
      principal_log_new_object_map & result_record,
      const filename & tmp_file,
      size_t offset,
      size_t size,
      const error_handler & on_error);

    bool generate_horcruxes(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      chunk_info & chunk_info,
      const std::vector<uint8_t> & buffer,
      const error_handler & on_error);

    bool generate_tail_horcruxes(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t chunk_index,
      const error_handler & on_error);

    void add_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      size_t size,
      const const_data_buffer & object_hash);

    void add_tail_object_chunk_map(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t chunk_index,
      const guid & object_id,
      size_t object_offset,
      size_t chunk_offset,
      const const_data_buffer & hash,
      const const_data_buffer & data);

    size_t get_last_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id);

    size_t get_tail_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t & result_size);

    void start_tail_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t chunk_index);

    void final_tail_chunk(
      const service_provider & sp,
      database_transaction & tr,
      size_t chunk_length,
      const const_data_buffer & chunk_hash,
      const guid & server_id,
      size_t chunk_index);

    void add_to_tail_chunk(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_id,
      size_t offset,
      const const_data_buffer & object_hash,
      const guid & server_id,
      size_t index,
      size_t chunk_offset,
      const const_data_buffer & data);

    void add_chunk_store(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t index,
      uint16_t replica,
      const guid & storage_id,
      const const_data_buffer & data);


    const_data_buffer get_tail_data(
      const service_provider & sp,
      database_transaction & tr,
      const guid & server_id,
      size_t chunk_index);

  };

  inline _chunk_manager * vds::ichunk_manager::operator->()
  {
    return static_cast<_chunk_manager *>(this);
  }
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
