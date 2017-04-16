#ifndef __VDS_PROTOCOLS_NODE_MANAGER_H_
#define __VDS_PROTOCOLS_NODE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class server;
  class _node_manager;

  class node_manager
  {
  public:
    ~node_manager();

    vds::async_task<> register_server(const service_provider & scope, const std::string & node_certificate);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

  private:
    friend class server;

    node_manager(_node_manager * impl);

    _node_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_NODE_MANAGER_H_
