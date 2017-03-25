#ifndef __VDS_SERVER_NODE_MANAGER_P_H_
#define __VDS_SERVER_NODE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _node_manager
  {
  public:
    _node_manager(const service_provider & sp);

    bool register_server(const service_provider & scope, const std::string & node_certificate, std::string & error);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);


  private:
    service_provider sp_;
    logger log_;
  };
}

#endif // __VDS_SERVER_NODE_MANAGER_P_H_
