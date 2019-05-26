#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_address.h"
#include "stream.h"

namespace vds {
  class _tcp_socket_server;
  class tcp_network_socket;
  
  class tcp_socket_server
  {
  public:
    tcp_socket_server();
    ~tcp_socket_server();

    vds::expected<void> start(
      const service_provider * sp,
      const network_address & address,
      lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>, std::shared_ptr<tcp_network_socket>> new_connection);
    
    void stop();
    
  private:
    std::shared_ptr<_tcp_socket_server> impl_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
