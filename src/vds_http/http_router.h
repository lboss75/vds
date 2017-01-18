/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef __VDS_HTTP_HTTP_ROUTER_H_
#define __VDS_HTTP_HTTP_ROUTER_H_

namespace vds {
  class http_request;
  class http_response;
  
  class http_router
  {
  public:
    void route(
      const simple_done_handler_t & done,
      const error_handler_t & on_error,
      std::shared_ptr<http_request> request,
      std::shared_ptr<http_response> response);
    
    void add_static(
      const std::string & url,
      const std::string & response);
    
  private:
    std::map<std::string, std::string> static_;
  };
}

#endif // HTTP_ROUTER_H
