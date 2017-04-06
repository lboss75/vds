#ifndef __VDS_SERVER_NODE_MANAGER_H_
#define __VDS_SERVER_NODE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _node_manager;

  class node_manager
  {
  public:
    node_manager(const service_provider & sp);
    ~node_manager();

    vds::async_task<> register_server(const service_provider & scope, const std::string & node_certificate);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

  private:
    std::unique_ptr<_node_manager> impl_;
  };
}

#endif // __VDS_SERVER_NODE_MANAGER_H_
