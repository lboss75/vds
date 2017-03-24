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

  private:
    service_provider sp_;
    logger log_;

    std::mutex states_mutex_;
  };
}

#endif // __VDS_SERVER_NODE_MANAGER_P_H_
