#ifndef __VDS_HTTP_HTTP_MIDDLEWARE_H_
#define __VDS_HTTP_HTTP_MIDDLEWARE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_router.h"
#include "http_outgoing_stream.h"

namespace vds {
  class http_router;
  class http_request;
  
  class http_middleware
  {
  public:
    http_middleware(const http_router & router)
    : router_(router){
    }
    

    template<
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
    class handler
    {
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const http_middleware & params)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method),
        router_(params.router_)
      {
      }
      
      void operator()(
        const http_request & request,
        http_incoming_stream & incoming_stream
      ) {
        this->response_.reset(request);
        this->router_.route(
          request,
          incoming_stream,
          this->response_,
          this->outgoing_stream_
        );
        
        this->next_method_(
          this->response_,
          this->outgoing_stream_);
      }
      
      void processed()
      {
        this->done_method_();
      }
      
    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;
      const http_router & router_;
      http_response response_;
      http_outgoing_stream outgoing_stream_;
    };
    
  private:
    const http_router & router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
