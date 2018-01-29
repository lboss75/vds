#ifndef __VDS_SERVER_SERVER_HTTP_API_P_H_
#define __VDS_SERVER_SERVER_HTTP_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_json_api.h"
#include "server_json_client_api.h"
#include "http_router.h"
#include "tcp_network_socket.h"
#include "tcp_socket_server.h"
#include "http_middleware.h"
#include "http_server.h"
#include "server_http_api.h"

namespace vds {
  class _server_http_api
    : public server_http_api,
      private http_router
  {
  public:
    _server_http_api();

    async_task<> start(
      const service_provider & sp,
      const std::string & address,
      int port,
      certificate & certificate,
      asymmetric_private_key & private_key);

    void stop(const service_provider & sp);

    //override http_router
    async_task<http_message> route(
      const service_provider & sp,
      const http_message & request) const;

  private:
    tcp_socket_server server_;
    http_middleware<_server_http_api> middleware_;
    server_json_client_api server_json_client_api_;
  };
}

#endif // __VDS_SERVER_SERVER_HTTP_API_P_H_
