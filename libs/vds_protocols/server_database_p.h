#ifndef __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
#define __VDS_PROTOCOLS_SERVER_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_database.h"

namespace vds {
  class istorage_log;
  
  class _server_database : public iserver_database
  {
  public:
    _server_database(server_database * owner);
    ~_server_database();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

    void add_cert(
      const service_provider & sp,
      const cert_record & record);

    std::unique_ptr<cert_record> find_cert(
      const service_provider & sp,
      const std::string & object_name);
    
    void add_object(
      const service_provider & sp,
      const guid & server_id,
      const server_log_new_object & index);
    
    uint64_t last_object_index(
      const service_provider & sp,
      const guid & server_id);
    
    void add_endpoint(
      const service_provider & sp,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      std::map<std::string, std::string> & addresses);

    void add_file(
      const service_provider & sp,
      const guid & server_id,
      const server_log_file_map & fm);

    void get_file_versions(
      const service_provider & sp,
      const std::string & user_login,
      const std::string & name,
      std::list<server_log_file_version> & result);

    void get_file_version_map(
      const service_provider & sp,
      const guid & server_id,
      const std::string & version_id,
      std::list<uint64_t> & result_indexes);


    server_log_record add_local_record(
      const service_provider & sp,
      const server_log_record::record_id & record_id,
      const json_value * message,
      const_data_buffer & signature);

    bool save_record(
      const service_provider & sp,
      const server_log_record & record,
      const const_data_buffer & signature);

    void processed_record(
      const service_provider & sp,
      const server_log_record::record_id & id);

    uint64_t get_server_log_max_index(
      const service_provider & sp,
      const guid & id);

    void get_unknown_records(
      const service_provider & sp,
      std::list<server_log_record::record_id> & result);

    bool get_record(
      const service_provider & sp,
      const server_log_record::record_id & id,
      server_log_record & result_record,
      const_data_buffer & result_signature);

    bool get_front_record(
      const service_provider & sp,
      server_log_record & result_record,
      const_data_buffer & result_signature);

    void delete_record(
      const service_provider & sp,
      const server_log_record::record_id & id);
    
    iserver_database::server_log_state server_log_get_state(
      const service_provider & sp,
      const server_log_record::record_id & record_id);
  private:
    server_database * owner_;
    database db_;

    std::mutex operation_mutex_;

    prepared_statement<
      const std::string & /* object_name */,
      const std::string & /* body */,
      const std::string & /* key */,
      const const_data_buffer & /*password_hash*/> add_cert_statement_;
      
    prepared_query<
      const std::string & /* object_name */> find_cert_query_;
      
    prepared_statement<
      const guid & /*server_id*/,
      uint64_t /*index*/,
      uint64_t /*original_lenght*/,
      const const_data_buffer & /*original_hash*/,
      uint64_t /*target_lenght*/,
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
      uint64_t /*index*/,
      int /*order*/> add_file_map_statement_;

    prepared_query<
      const std::string & /*user_login*/,
      const std::string & /*name*/> get_file_versions_query_;

    prepared_query<
      const std::string & /*version_id*/> get_file_version_map_query_;

    /// Server log
    std::mutex server_log_mutex_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      const guid & /* target_id*/,
      uint64_t /* target_index*/> server_log_add_link_statement_;

    void server_log_add_link(
      const service_provider & sp,
      const guid & source_id,
      uint64_t source_index,
      const guid & target_id,
      uint64_t target_index);


    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      const std::string & /* body */,
      const const_data_buffer & /* signature */,
      int /*state*/> server_log_add_statement_;

    void add_server_log(
      const service_provider & sp,
      const guid & source_id,
      uint64_t source_index,
      const std::string & body,
      const const_data_buffer & signature,
      iserver_database::server_log_state state);

    prepared_query<> get_server_log_tails_query_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      int /*state*/> server_log_update_state_statement_;

    void server_log_update_state(
      const service_provider & sp,
      const server_log_record::record_id & record_id,
      iserver_database::server_log_state state);

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> server_log_get_state_query_;

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> server_log_get_parents_query_;

    void server_log_get_parents(
      const service_provider & sp,
      const server_log_record::record_id & record_id,
      std::list<server_log_record::record_id> & parents);

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> server_log_get_followers_query_;

    void server_log_get_followers(
      const service_provider & sp,
      const server_log_record::record_id & record_id,
      std::list<server_log_record::record_id> & parents);

    prepared_query<
      const guid & /*source_id*/> get_server_log_max_index_query_;

    prepared_query<> get_unknown_records_query_;

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> server_log_get_query_;
      
    prepared_query<
      int /*state*/> get_record_by_state_query_;
      
    bool get_record_by_state(
      const service_provider & sp,
      iserver_database::server_log_state state,
      server_log_record & result_record,
      const_data_buffer & result_signature);
  };

}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
