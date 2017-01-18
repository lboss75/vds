#ifndef __VDS_HTTP_HTTP_MIDDLEWARE_H_
#define __VDS_HTTP_HTTP_MIDDLEWARE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_router.h"

namespace vds {
  class http_router;
  class http_request;
  
  class http_middleware
  {
  public:
    http_middleware(http_router * router)
    : router_(router){
    }
    
    class context
    {
    public:
      context(const http_middleware & params)
      : router_(params.router_) {
      }
      
      template<typename next_filter>
      void operator()(
        const simple_done_handler_t & done,
        const error_handler_t & on_error,
        next_filter next,
        const std::shared_ptr<http_request> & request
        ) {
        std::shared_ptr<http_response> response(
          new http_response(
            [&next](
              const simple_done_handler_t & done,
              const error_handler_t & on_error,
              const std::shared_ptr<http_response> & response
            ){
              next(
                done,
                on_error,
                response
              );
            },
            request
          ));
        
        this->router_->route(
          done,
          on_error,
          request,
          response
        );
      }
    private:
      http_router * router_;
    };
    
  private:
    http_router * router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
