#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "network_address.h"

namespace vds {
  class _tcp_socket_server;
  class tcp_network_socket;
  
  class tcp_socket_server
  {
  public:
    tcp_socket_server();
    ~tcp_socket_server();

    vds::async_task<vds::expected<void>> start(
      const service_provider * sp,
      const network_address & address,
      const std::function<vds::async_task<vds::expected<void>>(const std::shared_ptr<tcp_network_socket> & s)> & new_connection);
    
    void stop();
    
  private:
    std::shared_ptr<_tcp_socket_server> impl_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
