#ifndef __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
#define __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_stream_reader.h"

namespace vds {
  class http_simple_response_reader
  {
  public:
    http_simple_response_reader(std::string & body_collector)
    : body_collector_(body_collector)
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
        error_method_(error_method),
        body_collector_(args.body_collector_)
      {
      }

      void operator()(
        const http_response & response,
        http_incoming_stream & response_stream
        )
      {
        if(http_response::HTTP_OK == response.code()){
        }
      }

      void processed()
      {
      }

    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;
      std::string & body_collector_;
    };    
  private:
    std::string & body_collector_;
  };
}

#endif // __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
