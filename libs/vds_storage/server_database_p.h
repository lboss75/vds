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

  private:
    service_provider sp_;
    server_database * owner_;
    database db_;
  };

}

#endif // __VDS_STORAGE_SERVER_DATABASE_P_H_
