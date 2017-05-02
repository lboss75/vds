#ifndef __VDS_HTTP_HTTP_ROUTER_H_
#define __VDS_HTTP_HTTP_ROUTER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_request;
  class http_response;
  class http_incoming_stream;
  class http_outgoing_stream;
  
  class http_router
  {
  public:
    http_router();

    void route(
      const service_provider & sp,
      const http_request & request,
      http_incoming_stream & incoming_stream,
      http_response & response,
      http_outgoing_stream & outgoing_stream
      ) const;
    
    void add_static(
      const std::string & url,
      const std::string & response);

    void add_file(
      const std::string & url,
      const filename & filename);
    
        template<
      typename prev_handler_type,
      typename next_handler_type,
      typename error_handler_type
    >
    void route(
      const service_provider & sp,
      const http_request & request,
      http_incoming_stream & incoming_stream,
      http_response & response,
      http_outgoing_stream & outgoing_stream,
      prev_handler_type & prev_handler,
      next_handler_type & next_handler,
      error_handler_type & error_handler
    ) const
    {
      this->route(sp, request, incoming_stream, response, outgoing_stream);
      next_handler(sp, response, outgoing_stream);      
    }
    
  private:
    std::map<std::string, std::string> static_;
    std::map<std::string, filename> files_;
  };
}

#endif // __VDS_HTTP_HTTP_ROUTER_H_
