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
    

    template<typename context_type>
    class handler : public sequence_step<context_type, 
      void(
        http_response & response,
        http_outgoing_stream & outgoing_stream
      )
    >
    {
      using base_class = sequence_step<context_type, 
      void(
        http_response & response,
        http_outgoing_stream & outgoing_stream
      )>;
    public:
      handler(
        const context_type & context,
        const http_middleware & params)
        : base_class(context),
        router_(params.router_)
      {
      }
      
      void operator()(
        const http_request & request,
        http_incoming_stream & incoming_stream
      ) {
        this->response_.reset(request);
        if (!request.empty()) {
          this->router_.route(
            request,
            incoming_stream,
            this->response_,
            this->outgoing_stream_
          );
        }
        
        this->next(
          this->response_,
          this->outgoing_stream_);
      }
      
    private:
      const http_router & router_;
      http_response response_;
      http_outgoing_stream outgoing_stream_;
    };
    
  private:
    const http_router & router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
