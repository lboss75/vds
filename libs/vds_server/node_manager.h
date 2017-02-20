#ifndef __VDS_SERVER_NODE_MANAGER_H_
#define __VDS_SERVER_NODE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class json_array;
  class install_node_prepare;

  class node_manager
  {
  public:
    node_manager(const service_provider & sp);

    void install_prepate(json_array * result, const install_node_prepare & message);

  private:
    service_provider sp_;
    logger log_;

    std::mutex mutex_;
    std::list<std::string> processed_requests_;
  };
}

#endif // __VDS_SERVER_NODE_MANAGER_H_
