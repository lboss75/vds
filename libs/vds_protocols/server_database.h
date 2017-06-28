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
    

    void get_principal_log(
      const service_provider & sp,
      const guid & principal_id,
      size_t last_order_num,
      size_t & result_last_order_num,
      std::list<principal_log_record> & records);

    _server_database * operator -> ();
  };
}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_H_
