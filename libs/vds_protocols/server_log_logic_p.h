#ifndef __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_
#define __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_

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
    //return: true to process record
    bool add_record(const principal_log_record & record);

    void record_processed(const principal_log_record::record_id & id);

  private:
    service_provider sp_;
    principal_log_record::record_id current_id_;
  };
}

#endif // __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_
