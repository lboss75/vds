#ifndef __VDS_NETWORK_SOCKET_SERVER_P_H_
#define __VDS_NETWORK_SOCKET_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "mt_service.h"
#include "network_types_p.h"
#include "network_service_p.h"
#include "tcp_network_socket_p.h"

namespace vds {
  class _tcp_socket_server
  {
  public:
    _tcp_socket_server();
    
    ~_tcp_socket_server();

    vds::expected<void> start(
      const service_provider * sp,
      const network_address & address,
      lambda_holder_t<vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>>,
      std::shared_ptr<tcp_network_socket>> new_connection);
    
    void stop();
    
  private:
    SOCKET_HANDLE s_;
    std::thread wait_accept_task_;
    bool is_shuting_down_;

#ifndef _WIN32
#else
    class windows_wsa_event
    {
    public:
      windows_wsa_event();

      ~windows_wsa_event();

      expected<void> select(SOCKET s, long lNetworkEvents);

      WSAEVENT handle() const {
        return this->handle_;
      }

    private:
      WSAEVENT handle_;
    };

    windows_wsa_event accept_event_;
#endif
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_P_H_
