#ifndef __VDS_STORAGE_SERVER_DATABASE_H_
#define __VDS_STORAGE_SERVER_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"

namespace vds {
  class _server_database;
  class cert;

  class server_database
  {
  public:
    server_database(const service_provider & sp);
    ~server_database();

    void start();
    void stop();
    
    void add_cert(const cert & record);
    std::unique_ptr<cert> find_cert(const std::string & object_name) const;
    
    void add_object(
      const guid & server_id,
      const chunk_manager::object_index & index);
    
    uint64_t last_object_index(
      const guid & server_id);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

  private:
    _server_database * const impl_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_H_
