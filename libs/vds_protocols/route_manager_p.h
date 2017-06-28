#ifndef __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_
#define __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "route_manager.h"

namespace vds {
  
  class _route_manager
  {
  public:
    _route_manager();
    ~_route_manager();
    
    void on_session_started(
      const service_provider& sp,
      const guid & source_server_id,
      const guid & target_server_id,
      const std::string & address);

  private:
    
  };
}

#endif // __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_
