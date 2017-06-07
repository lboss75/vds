#ifndef __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
#define __VDS_PROTOCOLS_SERVER_DATABASE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_database.h"
#include "database.h"

namespace vds {
  class istorage_log;
  
  class _server_database : public iserver_database
  {
  public:
    _server_database();
    ~_server_database();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

    void add_principal(
      const service_provider & sp,
      const principal_record & record);

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
      const guid & server_id,
      const principal_log_new_object & index);
    
    uint64_t last_object_index(
      const service_provider & sp,
      const guid & server_id);
    
    void add_endpoint(
      const service_provider & sp,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      std::map<std::string, std::string> & addresses);

    principal_log_record add_local_record(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      const std::shared_ptr<json_value> & message,
      const_data_buffer & signature);

    bool save_record(
      const service_provider & sp,
      const principal_log_record & record,
      const const_data_buffer & signature);

    void processed_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);

    uint64_t get_principal_log_max_index(
      const service_provider & sp,
      const guid & id);

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

    void delete_record(
      const service_provider & sp,
      const principal_log_record::record_id & id);
    
    iserver_database::principal_log_state principal_log_get_state(
      const service_provider & sp,
      const principal_log_record::record_id & record_id);
  private:
    database db_;

    std::mutex operation_mutex_;

    prepared_statement<
      const guid & /* object_name */,
      const std::string & /* body */,
      const std::string & /* key */,
      const const_data_buffer & /*password_hash*/> add_principal_statement_;
      
    prepared_query<
      const guid & /* object_name */> find_principal_query_;
      
    prepared_statement<
      const guid & /* object_name */,
      const std::string & /* login */> add_user_principal_statement_;

    prepared_query<
      const std::string & /* object_name */> find_user_principal_query_;

    prepared_statement<
      const guid & /*server_id*/,
      uint64_t /*index*/,
      uint64_t /*lenght*/,
      const const_data_buffer & /*hash*/,
      const guid & /*owner_principal*/> add_object_statement_;
      
    prepared_query<
      const guid & /*server_id*/> last_object_index_query_;
      
    prepared_statement<
      const std::string & /*endpoint_id*/,
      const std::string & /*addresses*/> add_endpoint_statement_;
      
    prepared_query<> get_endpoints_query_;
    

    /// Server log
    std::mutex principal_log_mutex_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      const guid & /* target_id*/,
      uint64_t /* target_index*/> principal_log_add_link_statement_;

    void principal_log_add_link(
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
      int /*state*/> principal_log_add_statement_;

    void add_principal_log(
      const service_provider & sp,
      const guid & record_id,
      const std::string & body,
      const const_data_buffer & signature,
      iserver_database::principal_log_state state);

    prepared_query<> get_principal_log_tails_query_;

    prepared_statement<
      const guid & /*source_id*/,
      uint64_t /*source_index*/,
      int /*state*/> principal_log_update_state_statement_;

    void principal_log_update_state(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      iserver_database::principal_log_state state);

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> principal_log_get_state_query_;

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> principal_log_get_parents_query_;

    void principal_log_get_parents(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      std::list<principal_log_record::record_id> & parents);

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> principal_log_get_followers_query_;

    void principal_log_get_followers(
      const service_provider & sp,
      const principal_log_record::record_id & record_id,
      std::list<principal_log_record::record_id> & parents);

    prepared_query<
      const guid & /*source_id*/> get_principal_log_max_index_query_;

    prepared_query<> get_unknown_records_query_;

    prepared_query<
      const guid & /*source_id*/,
      uint64_t /*source_index*/> principal_log_get_query_;
      
    prepared_query<
      int /*state*/> get_record_by_state_query_;
      
    bool get_record_by_state(
      const service_provider & sp,
      iserver_database::principal_log_state state,
      principal_log_record & result_record,
      const_data_buffer & result_signature);
  };

}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_P_H_
