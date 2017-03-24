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

  private:
    service_provider sp_;
    server_database * owner_;
    database db_;

    prepared_statement<
      const std::string & /* object_name */,
      const guid & /* source_id */,
      uint64_t /* index */,
      const data_buffer & /* signature */,
      const data_buffer & /*password_hash*/> add_cert_statement_;
      
    prepared_query<
      const std::string & /* object_name */> find_cert_query_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_P_H_
