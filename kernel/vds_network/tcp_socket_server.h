#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"

namespace vds {
  class _tcp_socket_server;
  class tcp_network_socket;
  
  class tcp_socket_server
  {
  public:
    tcp_socket_server();
    ~tcp_socket_server();

    async_task<> start(
      const service_provider & sp,
      const std::string & address,
      int port,
      const std::function<void(const service_provider & sp, const tcp_network_socket & s)> & new_connection);
    
    void stop(const service_provider & sp);
    
  private:
    std::shared_ptr<_tcp_socket_server> impl_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
