#ifndef __VDS_WEB_SERVER_WEB_SERVER_P_H_
#define __VDS_WEB_SERVER_WEB_SERVER_P_H_
#include "tcp_socket_server.h"
#include "http_middleware.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_message;

  class _web_server : public std::enable_shared_from_this<_web_server>
  {
  public:
    _web_server(const service_provider & sp);
    ~_web_server();
    
    void start(const service_provider& sp);
    async_task<> prepare_to_stop(const service_provider &sp);

    async_task<http_message> route(
      const service_provider & sp,
      const http_message & request) const;

  private:
    tcp_socket_server server_;
    http_middleware<_web_server> middleware_;
    http_router router_;
  };
}

#endif // __VDS_WEB_SERVER_WEB_SERVER_P_H_
