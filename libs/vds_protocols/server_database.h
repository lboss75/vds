#ifndef __VDS_PROTOCOLS_SERVER_DATABASE_H_
#define __VDS_PROTOCOLS_SERVER_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"

namespace vds {
  class _server_database;
  class principal_record;

  class iserver_database
  {
  public:
    void add_principal(
      const service_provider & sp,
      const principal_record & record);

    guid get_root_principal(
      const service_provider & sp);

    void add_user_principal(
      const service_provider & sp,
      const std::string & login,
      const principal_record & record);

    std::unique_ptr<principal_record> find_principal(
      const service_provider & sp,
      const guid & object_name);

    std::unique_ptr<principal_record> find_user_principal(
      const service_provider & sp, 
      const std::string & object_name);

    void add_object(
      const service_provider & sp,
      const principal_log_new_object & index);

    void add_endpoint(
      const service_provider & sp,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      const service_provider & sp,
      std::map<std::string, std::string> & addresses);

    principal_log_record add_local_record(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      const guid & principal_id,
      const std::shared_ptr<json_value> & message,
      const vds::asymmetric_private_key & principal_private_key,
      const_data_buffer & signature);

    size_t get_current_state(
      const service_provider & sp,
      std::list<guid> & active_records);

    //return: true - saved, false - already exists
    bool save_record(
      const service_provider & sp,
      const principal_log_record & record,
      const const_data_buffer & signature);

    enum class principal_log_state
    {
      not_found = 0,
      stored = 1, //Just stored
      front = 2, //Stored + all parents processed
      processed = 3, //+ Processed
      tail = 4 //
    };
    
    principal_log_state get_record_state(
      const service_provider & sp,
      const principal_log_record::record_id & id);

    void get_unknown_records(
      const service_provider & sp,
      std::list<principal_log_record::record_id> & result);

    bool get_record(
      const service_provider & sp,
      const principal_log_record::record_id & id,
      principal_log_record & result_record,
      const_data_buffer & result_signature);
    
    bool get_front_record(
      const service_provider & sp,
      principal_log_record & result_record,
      const_data_buffer & result_signature);

    void processed_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);

    void delete_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);
    
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
      uint16_t replica,
      size_t replica_length,
      const const_data_buffer & replica_hash);

    void add_chunk_store(
      const service_provider & sp,
      const guid & server_id,
      size_t index,
      uint16_t replica,
      const guid & storage_id,
      const const_data_buffer & replica_data);

    const_data_buffer get_tail_data(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index);
    
    void add_object_chunk_map(
      const service_provider & sp,
      const guid & server_id,
      size_t chunk_index,
      const guid & object_id,
      size_t object_offset,
      size_t chunk_offset,
      size_t length,
      const const_data_buffer & hash);

    struct object_chunk_map
    {
      guid server_id;
      size_t chunk_index;
      guid object_id;
      size_t object_offset;
      size_t chunk_offset;
      size_t length;
      const_data_buffer hash;
    };

    std::list<object_chunk_map> get_object_map(
      const service_provider & sp,
      const guid & object_id);

    void get_principal_log(
      const service_provider & sp,
      const guid & principal_id,
      size_t last_order_num,
      size_t & result_last_order_num,
      std::list<principal_log_record> & records);
  };
}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_H_
