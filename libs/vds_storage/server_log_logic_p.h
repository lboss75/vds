#ifndef __VDS_STORAGE_SERVER_LOG_LOGIC_P_H_
#define __VDS_STORAGE_SERVER_LOG_LOGIC_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_log_logic.h"
#include "server_database.h"

namespace vds {
  class _server_log_logic
  {
  public:
    void add_record(const server_log_record & record);

  private:
    service_provider sp_;
    lazy_service<iserver_database> db_;
    server_log_record::record_id current_id_;


  };
}

#endif // __VDS_STORAGE_SERVER_LOG_LOGIC_P_H_
