#ifndef __VDS_SERVER_SERVER_HTTP_API_P_H_
#define __VDS_SERVER_SERVER_HTTP_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_json_api.h"
#include "server_json_client_api.h"

namespace vds {
  class _server_http_api : public server_http_api
  {
  public:
    _server_http_api();

    async_task<> start(
      const service_provider & sp,
      const std::string & address,
      int port,
      certificate & certificate,
      asymmetric_private_key & private_key);

  private:
    std::unique_ptr<http_router> router_;
    
    class server_http_handler
    {
    public:
      server_http_handler(
        const http_router & router
      );
      
      template<
        typename prev_handler_type,
        typename next_handler_type,
        typename error_handler_type
      >
      void route(
        const service_provider & scope,
        const http_request & request,
        http_incoming_stream & incoming_stream,
        http_response & response,
        http_outgoing_stream & outgoing_stream,
        prev_handler_type & prev_handler,
        next_handler_type & next_handler,
        error_handler_type & error_handler
      ) const
      {
        if("/vds/client_api" == request.url()){
          dataflow(
            http_stream_reader<prev_handler_type>(prev_handler, incoming_stream),
            json_parser("client_api"),
            http_json_api<server_json_client_api>(scope, this->server_json_client_api_),
            http_json_formatter(response, outgoing_stream)
          )(
            next_handler,
            error_handler
          );
        }
        else {
          this->router_.route<prev_handler_type, next_handler_type, error_handler_type>(
            scope,
            request,
            incoming_stream,
            response,
            outgoing_stream,
            prev_handler,
            next_handler,
            error_handler
          );
        }
      }
    private:
      server_json_api server_json_api_;
      server_json_client_api server_json_client_api_;
      const http_router & router_;
    };


    class socket_session
    {
    public:
      socket_session(
        const http_router & router,
        const certificate & certificate,
        const asymmetric_private_key & private_key
      );

      class handler
      {
      public:
        handler(
          const socket_session & owner,
          network_socket & s);

        void start(const service_provider & sp);

      private:
        network_socket s_;
        ssl_tunnel tunnel_;
        const certificate & certificate_;
        const asymmetric_private_key & private_key_;
        server_http_handler server_http_handler_;
        delete_this<handler> done_handler_;

        std::function<void(const service_provider & sp, std::exception_ptr)> error_handler_;

        std::function<void(const service_provider & sp)> http_server_done_;
        std::function<void(const service_provider & sp, std::exception_ptr)> http_server_error_;
      };
    private:
      const http_router & router_;
      const certificate & certificate_;
      const asymmetric_private_key & private_key_;

    };
  };
}

#endif // __VDS_SERVER_SERVER_HTTP_API_P_H_
