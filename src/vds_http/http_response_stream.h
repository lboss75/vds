#ifndef __VDS_HTTP_HTTP_RESPONSE_STREAM_H_
#define __VDS_HTTP_HTTP_RESPONSE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"

namespace vds {
  
  class http_response_stream
  {
  public:
    class context
    {
    public:
      context(const http_response_stream & params) {
      }
      
      template<typename next_filter>
      void operator()(
        const simple_done_handler_t & done,
        const error_handler_t & on_error,
        next_filter next,
        const std::shared_ptr<http_response> & response
        ){
        auto result = new std::string(response->result());
        next(
          [result, done](){
            delete result;
            done();
          },
          on_error,
          result->c_str(),
          result->length()
        );
      }
    };
    
  private:
  };
}

#endif // __HTTP_RESPONSE_STREAM_H_
