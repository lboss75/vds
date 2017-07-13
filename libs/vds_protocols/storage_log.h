#ifndef __VDS_PROTOCOLS_STORAGE_LOG_H_
#define __VDS_PROTOCOLS_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "log_records.h"

namespace vds {
  class _storage_log;
  class endpoint;
  class istorage_log;
  class full_storage_object_id;
  class principal_log_record;
  class server;
  
  class istorage_log
  {
  public:
    const guid & current_server_id() const;
    const certificate & server_certificate() const;
    const asymmetric_private_key & server_private_key() const;
    
    size_t minimal_consensus() const;

    void add_to_local_log(
      const service_provider & sp,
      database_transaction & tr,
      const guid & principal_id,
      const vds::asymmetric_private_key & principal_private_key,
      const std::shared_ptr<json_value> & record,
      bool apply_record = true);

    vds::async_task<> register_server(
      const service_provider & sp,
      database_transaction & tr,
      const guid & id,
      const guid & parent_id,
      const std::string & server_certificate,
      const std::string & server_private_key,
      const const_data_buffer & password_hash);

    std::unique_ptr<const_data_buffer> get_object(
      const service_provider & sp,
      database_transaction & tr,
      const full_storage_object_id & object_id);

    void add_endpoint(
      const service_provider & sp,
      database_transaction & tr,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      const service_provider & sp,
      database_transaction & tr,
      std::map<std::string, std::string> & addresses);

    void reset(
      const service_provider & sp,
      const guid & principal_id,
      const certificate & root_certificate,
      const asymmetric_private_key & private_key,
      const std::string & root_password,
      const std::string & address);

    void apply_record(
      const service_provider & sp,
      database_transaction & tr,
      const principal_log_record & record,
      const const_data_buffer & signature);

    principal_log_record::record_id get_last_applied_record(
      const service_provider & sp);
  };
}

#endif // __VDS_PROTOCOLS_STORAGE_LOG_H_
