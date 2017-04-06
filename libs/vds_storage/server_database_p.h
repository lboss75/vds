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

    void get_endpoints(std::map<std::string, std::string> & addresses);

    void add_file(
      const guid & server_id,
      const server_log_file_map & fm);

  private:
    service_provider sp_;
    server_database * owner_;
    database db_;

    prepared_statement<
      const std::string & /* object_name */,
      const guid & /* source_id */,
      uint64_t /* index */,
      const data_buffer & /*password_hash*/> add_cert_statement_;
      
    prepared_query<
      const std::string & /* object_name */> find_cert_query_;
      
    prepared_statement<
      const guid & /*server_id*/,
      uint64_t /*index*/,
      uint32_t /*original_lenght*/,
      const data_buffer & /*original_hash*/,
      uint32_t /*target_lenght*/,
      const data_buffer & /*signature*/> add_object_statement_;
      
    prepared_query<
      const guid & /*server_id*/> last_object_index_query_;
      
    prepared_statement<
      const std::string & /*endpoint_id*/,
      const std::string & /*addresses*/> add_endpoint_statement_;
      
    prepared_query<> get_endpoints_query_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_P_H_
