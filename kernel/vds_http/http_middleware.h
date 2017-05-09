#ifndef __VDS_HTTP_HTTP_MIDDLEWARE_H_
#define __VDS_HTTP_HTTP_MIDDLEWARE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_request.h"
#include "http_response.h"
#include "http_router.h"
#include "http_outgoing_stream.h"

namespace vds {
  class http_router;
  
  template <typename router_type>
  class http_middleware
  {
  public:
    http_middleware(const router_type & router)
    : router_(router){
    }
    

    template<typename context_type>
    class handler : public dataflow_step<context_type, 
      bool(
        http_response & response,
        http_outgoing_stream & outgoing_stream
      )
    >
    {
      using base_class = dataflow_step<context_type, 
      bool(
        http_response & response,
        http_outgoing_stream & outgoing_stream
      )>;
    public:
      handler(
        const context_type & context,
        const http_middleware & params)
        : base_class(context),
        router_(params.router_),
        http_error_handler_(this)
      {
      }
      
      bool operator()(
        const service_provider & sp,
        const http_request & request,
        http_incoming_stream & incoming_stream
      ) {
        
        this->response_.reset(request);
        if (!request.empty()) {
          try {
            return this->router_.route(
              sp,
              request,
              incoming_stream,
              this->response_,
              this->outgoing_stream_,
              this->prev,
              this->next,
              this->http_error_handler_
            );
          }
          catch(...) {
            this->http_error_handler_(sp, std::current_exception());
            return false;
          }
        }
        else {
          return this->next(
            sp,
            this->response_,
            this->outgoing_stream_);
        }
      }
      
    private:
      const router_type & router_;
      http_response response_;
      http_outgoing_stream outgoing_stream_;
      
      class http_error_handler
      {
      public:
        http_error_handler(handler * owner)
        : owner_(owner)
        {
        }
        
        void operator()(const service_provider & sp, std::exception_ptr ex)
        {
          this->owner_->response_.set_result(
            http_response::HTTP_Internal_Server_Error,
            exception_what(ex));
          this->owner_->outgoing_stream_.set_body(exception_what(ex));
          this->owner_->next(
            sp,
            this->owner_->response_,
            this->owner_->outgoing_stream_);
        }
      private:
        handler * owner_;
      };
      http_error_handler http_error_handler_;
    };
    
  private:
    const router_type & router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
