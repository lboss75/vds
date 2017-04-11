#ifndef __VDS_STORAGE_SERVER_DATABASE_P_H_
#define __VDS_STORAGE_SERVER_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_database.h"

namespace vds {
  
  class _server_database
  {
  public:
    _server_database(const service_provider & sp, server_database * owner);
    ~_server_database();

    void start();
    void stop();

    void add_cert(const cert & record);
    std::unique_ptr<cert> find_cert(const std::string & object_name);
    
    void add_object(
      const guid & server_id,
      const server_log_new_object & index);
    
    uint64_t last_object_index(
      const guid & server_id);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      std::map<std::string, std::string> & addresses);

    void add_file(
      const guid & server_id,
      const server_log_file_map & fm);
    
    void add_log_record(
      const guid & server_id,
      const std::string & record,
      const const_data_buffer & signature);

    server_log_record
      add_local_record(
        const server_log_record::record_id & record_id,
        const json_value * message,
        const_data_buffer & signature);
      bool have_log_record(const server_log_record::record_id & id);


  private:
    service_provider sp_;
    server_database * owner_;
    database db_;

    std::mutex operation_mutex_;

    prepared_statement<
      const std::string & /* object_name */,
      const guid & /* source_id */,
      uint64_t /* index */,
      const const_data_buffer & /*password_hash*/> add_cert_statement_;
      
    prepared_query<
      const std::string & /* object_name */> find_cert_query_;
      
    prepared_statement<
      const guid & /*server_id*/,
      uint64_t /*index*/,
      uint32_t /*original_lenght*/,
      const const_data_buffer & /*original_hash*/,
      uint32_t /*target_lenght*/,
      const const_data_buffer & /*signature*/> add_object_statement_;
      
    prepared_query<
      const guid & /*server_id*/> last_object_index_query_;
      
    prepared_statement<
      const std::string & /*endpoint_id*/,
      const std::string & /*addresses*/> add_endpoint_statement_;
      
    prepared_query<> get_endpoints_query_;
    
    prepared_statement<
      const std::string & /*version_id*/,
      const guid & /*server_id*/,
      const std::string & /*user_login*/,
      const std::string & /*name*/> add_file_statement_;
      
    prepared_statement<
      const std::string & /*version_id*/,
      uint64_t /*index*/> add_file_map_statement_;

    prepared_query<> log_parents_query_;
    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> update_server_log_tail_statement_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      const guid & /* target_id*/,
      uint64_t /* target_index*/> add_server_log_link_statement_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      const std::string & /* body */,
      const const_data_buffer & /* signature */> add_server_log_statement_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_P_H_
