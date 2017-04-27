#ifndef __VDS_STORAGE_SERVER_LOG_LOGIC_H_
#define __VDS_STORAGE_SERVER_LOG_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "log_records.h"

namespace vds {
  class server_log_logic
  {
  public:
    server_log_logic();

    void add_record(const server_log_record & record);

  private:
  };
}

#endif // __VDS_STORAGE_SERVER_LOG_LOGIC_H_
