#ifndef __VDS_SERVER_SERVER_LOGIC_H_
#define __VDS_SERVER_SERVER_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "server_json_api.h"

namespace vds {
  class server_logic
  {
  public:
    server_logic(
      const service_provider & sp,
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
      if("/vds/ping" == request.url()){
        sequence(
          http_stream_reader<prev_handler_type>(prev_handler, incoming_stream),
          json_parser("ping request"),
          http_json_api<server_json_api>(scope, this->server_json_api_),
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
    const http_router & router_;
  };
}

#endif // __VDS_SERVER_SERVER_LOGIC_H_
