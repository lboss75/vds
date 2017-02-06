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
    http_router(const service_provider & sp);

    void route(
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

  private:
    logger log_;
    std::map<std::string, std::string> static_;
    std::map<std::string, filename> files_;
  };
}

#endif // __VDS_HTTP_HTTP_ROUTER_H_
