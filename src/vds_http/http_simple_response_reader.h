#ifndef __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
#define __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_stream_reader.h"
#include "http_incoming_stream.h"

namespace vds {
  class http_simple_response_reader
  {
  public:
    http_simple_response_reader()
    {
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
        const http_simple_response_reader & args)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method)
      {
      }

      void operator()(
        const http_response & response,
        http_incoming_stream & response_stream
        )
      {
        if(http_response::HTTP_OK == response.code()){
          pipeline(
            http_stream_reader(response_stream),
            stream_to_string()
          )
          (
           this->next_method_,
           this->error_method_
          );
          
          this->done_method_();
        }
      }

      void processed()
      {
        this->done_method_();
      }

    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;
    };    
  };
}

#endif // __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
