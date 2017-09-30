#ifndef __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_
#define __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "route_manager.h"

namespace vds {
  class _connection_manager;
  class connection_session;
  
  class _route_manager
  {
  public:
    _route_manager();
    ~_route_manager();

    void send_to(
      const service_provider & sp,
      const guid & server_id,
      uint32_t message_type_id,
      const const_data_buffer & message_data);

    
    void on_session_started(
      const service_provider& sp,
      const guid & source_server_id,
      const guid & target_server_id,
      const std::string & address);

    void on_route_message(
      _connection_manager * con_man,
      const service_provider & sp,
      database_transaction & t,
      const connection_session & session,
      const route_message & message);

  private:
    std::mutex processed_route_message_mutex_;
    simple_cache<guid, guid> processed_route_message_;
  };
}

#endif // __VDS_PROTOCOLS_ROUTE_MANAGER_P_H_
