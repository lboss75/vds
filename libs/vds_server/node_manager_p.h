#ifndef __VDS_PROTOCOLS_NODE_MANAGER_P_H_
#define __VDS_PROTOCOLS_NODE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "node_manager.h"

namespace vds {
  class _node_manager : public node_manager
  {
  public:
    _node_manager();

    async_task<> register_server(
      const service_provider & sp,
      database_transaction & tr,
      const guid & id,
      const guid & parent_id,
      const certificate & server_certificate,
      const const_data_buffer & server_private_key,
      const const_data_buffer & password_hash);
    
    void add_endpoint(
      const service_provider & sp,
      database_transaction & tr,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      const service_provider & sp,
      database_transaction & tr,
      std::map<std::string, std::string> & addresses);
  };
}

#endif // __VDS_PROTOCOLS_NODE_MANAGER_P_H_
