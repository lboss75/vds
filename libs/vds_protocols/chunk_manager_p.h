#ifndef __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
#define __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "server_database.h"
#include "chunk_storage.h"

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
      const guid & version_id,
      const filename & tmp_file,
      const const_data_buffer & file_hash);

    static void create_database_objects(
      const service_provider & sp,
      uint64_t db_version,
      database & db);

    std::list<object_chunk_map> get_object_map(
      const service_provider & sp,
      const guid & object_id);

    void get_replicas(
      const service_provider & sp,
      const guid & server_id,
      ichunk_manager::index_type index,
      const guid & storage_id,
      std::list<ichunk_manager::replica_type> & result);

    const_data_buffer get_replica_data(
      const service_provider & sp,
      const guid & server_id,
      ichunk_manager::index_type index,
      const guid & storage_id,
      ichunk_manager::replica_type replica);

    void get_chunk_store(
      const service_provider & sp,
      const guid & server_id,
      index_type index,
      std::list<chunk_store> & result);

  private:
    chunk_storage chunk_storage_;

    std::mutex chunk_mutex_;
    uint64_t last_chunk_;
    
    std::mutex tail_chunk_mutex_;
    uint64_t tail_chunk_index_;
    size_t tail_chunk_size_;

    database * db_;
    
    static const size_t BLOCK_SIZE = 16 * 1024 * 1024;
    static const uint16_t MIN_HORCRUX = 512;
    static const uint16_t GENERATE_HORCRUX = 1024;
    static const uint16_t REPLICA_SIZE = BLOCK_SIZE / MIN_HORCRUX;
    
    bool write_chunk(
      const service_provider & sp,
      principal_log_new_object_map & result_record,
      const filename & tmp_file,
      size_t offset,
      size_t size,
      const error_handler & on_error);

    bool write_tail(
      const service_provider & sp,
      principal_log_new_object_map & result_record,
      const filename & tmp_file,
      size_t offset,
      size_t size,
      const error_handler & on_error);

    bool generate_horcruxes(
      const service_provider & sp,
      const guid & server_id,
      chunk_info & chunk_info,
      const std::vector<uint8_t> & buffer,
      const error_handler & on_error);

    bool generate_tail_horcruxes(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index,
      const error_handler & on_error);

    /// Database
    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/,
      size_t /*size*/,
      const const_data_buffer & /*object_hash*/> add_chunk_statement_;

    void add_chunk(
      const service_provider & sp,
      const guid & server_id,
      size_t index,
      size_t size,
      const const_data_buffer & object_hash);

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*chunk_index*/,
      const guid & /*object_id*/,
      size_t /*object_offset*/,
      size_t /*chunk_offset*/,
      size_t /*length*/,
      const const_data_buffer & /*hash*/> object_chunk_map_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/> add_tmp_chunk_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/> move_object_chunk_map_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/> delete_tmp_object_chunk_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/> delete_tmp_object_chunk_map_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*chunk_index*/,
      const guid & /*object_id*/,
      size_t /*object_offset*/,
      size_t /*chunk_offset*/,
      const const_data_buffer & /*hash*/,
      const const_data_buffer & /*data*/> tmp_object_chunk_map_statement_;

    void add_tail_object_chunk_map(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index,
      const guid & object_id,
      size_t object_offset,
      size_t chunk_offset,
      const const_data_buffer & hash,
      const const_data_buffer & data);

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/,
      uint16_t /*replica*/,
      size_t /*replica_length*/,
      const const_data_buffer & /*replica_hash*/> add_chunk_replica_statement_;

    prepared_statement<
      const guid & /*server_id*/,
      size_t /*index*/,
      uint16_t /*replica*/,
      const guid & /*storage_id*/,
      const const_data_buffer & /*data*/> add_object_chunk_store_statement_;

    prepared_query<
      const guid & /*server_id*/,
      size_t /*index*/> get_tail_data_query_;

    prepared_query<
      const guid & /*object_id*/> get_object_map_query_;

    prepared_query<
      const guid & /*principal_id*/,
      size_t /*last_order_num*/> get_principal_log_query_;

    prepared_query<
      const guid & /*server_id*/,
      size_t /*index*/,
      const guid & /*storage_id*/> get_replicas_query_;

    prepared_query<
      const guid & /*server_id*/,
      size_t /*index*/,
      const guid & /*storage_id*/,
      uint16_t /*replica*/> get_replica_data_query_;

    size_t get_last_chunk(
      const service_provider & sp,
      const guid & server_id);

    size_t get_tail_chunk(
      const service_provider & sp,
      const guid & server_id,
      size_t & result_size);

    void add_full_chunk(
      const service_provider & sp,
      const guid & object_id,
      size_t offset,
      size_t size,
      const const_data_buffer & object_hash,
      const guid & server_id,
      size_t index);

    void start_tail_chunk(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index);

    void final_tail_chunk(
      const service_provider & sp,
      size_t chunk_length,
      const const_data_buffer & chunk_hash,
      const guid & server_id,
      size_t chunk_index);

    void add_to_tail_chunk(
      const service_provider & sp,
      const guid & object_id,
      size_t offset,
      const const_data_buffer & object_hash,
      const guid & server_id,
      size_t index,
      size_t chunk_offset,
      const const_data_buffer & data);

    void add_chunk_replica(
      const service_provider & sp,
      const guid & server_id,
      size_t index,
      ichunk_manager::replica_type replica,
      size_t replica_length,
      const const_data_buffer & replica_hash);

    void add_chunk_store(
      const service_provider & sp,
      const guid & server_id,
      size_t index,
      uint16_t replica,
      const guid & storage_id,
      const const_data_buffer & data);


    const_data_buffer get_tail_data(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index);

    void add_object_chunk_map(
      const service_provider & sp,
      const guid & server_id,
      ichunk_manager::index_type chunk_index,
      const guid & object_id,
      size_t object_offset,
      size_t chunk_offset,
      size_t length,
      const const_data_buffer & hash);
  };

  inline _chunk_manager * vds::ichunk_manager::operator->()
  {
    return static_cast<_chunk_manager *>(this);
  }
}

#endif // __VDS_PROTOCOLS_CHUNK_MANAGER_P_H_
