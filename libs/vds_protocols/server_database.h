#ifndef __VDS_PROTOCOLS_SERVER_DATABASE_H_
#define __VDS_PROTOCOLS_SERVER_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "messages.h"

namespace vds {
  class _server_database;
  class principal_record;

  class iserver_database
  {
  public:
    void add_object(
      const service_provider & sp,
      database_transaction & tr,
      const principal_log_new_object & index);

    void add_endpoint(
      const service_provider & sp,
      database_transaction & tr,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      const service_provider & sp,
      database_transaction & tr,
      std::map<std::string, std::string> & addresses);


    _server_database * operator -> ();
  };
}

#endif // __VDS_PROTOCOLS_SERVER_DATABASE_H_
