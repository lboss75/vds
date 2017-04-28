#ifndef __VDS_PROTOCOLS_NODE_MANAGER_H_
#define __VDS_PROTOCOLS_NODE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class node_manager
  {
  public:
    vds::async_task<> register_server(
      const service_provider & sp,
      const std::string & node_certificate);
    
    void add_endpoint(
      const service_provider & sp,
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(
      const service_provider & sp,
      std::map<std::string, std::string> & addresses);
  };
}

#endif // __VDS_PROTOCOLS_NODE_MANAGER_H_
