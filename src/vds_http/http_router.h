/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef __VDS_HTTP_HTTP_ROUTER_H_
#define __VDS_HTTP_HTTP_ROUTER_H_

namespace vds {
  class http_request;
  class http_response;
  class http_incoming_stream;
  class http_outgoing_stream;
  
  class http_router
  {
  public:
    void route(
      const http_request & request,
      http_incoming_stream & incoming_stream,
      http_response & response,
      http_outgoing_stream & outgoing_stream
      );
    
    void add_static(
      const std::string & url,
      const std::string & response);
    
  private:
    std::map<std::string, std::string> static_;
  };
}

#endif // HTTP_ROUTER_H
