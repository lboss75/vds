#ifndef __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
#define __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "connection_manager.h"

namespace vds {
  
  class _server_log_sync
  {
  public:
    _server_log_sync(
      const service_provider & sp,
      server_log_sync * owner);
    
    ~_server_log_sync();
    
    void start();
    void stop();

  private:
    service_provider sp_;
    server_log_sync * const owner_;
    
    event_handler<
      const server_log_record & /*record*/,
      const const_data_buffer & /*signature*/> new_local_record;
      
    lazy_service<iconnection_manager> connection_manager_;
      
    void on_new_local_record(const server_log_record & record, const const_data_buffer & signature);
    
  };
}

#endif // __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
