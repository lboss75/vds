#ifndef __VDS_HTTP_HTTP_STREAM_READER_H_
#define __VDS_HTTP_HTTP_STREAM_READER_H_

#include <vector>
#include "http_incoming_stream.h"

namespace vds {
  class http_stream_reader
  {
  public:
    http_stream_reader(
      http_incoming_stream & incoming_stream
    )
    : incoming_stream_(incoming_stream)
    {
    }
    
    template<
      typename next_method_type,
      typename error_method_type
    >
    class handler : public http_incoming_stream::read_handler
    {
    public:
      handler(
        next_method_type & next_method,
        error_method_type & error_method,
        const http_stream_reader & args
      )
      : next_method_(next_method),
        error_method_(error_method),
        incoming_stream_(args.incoming_stream_)
      {
        this->incoming_stream_.handler(this);
      }
      
      void operator()()
      {
      }
      
      void push_data(
        const void * data,
        size_t len
      ){
        this->next_method_(data, len);
      }
      
    private:
      next_method_type & next_method_;
      error_method_type & error_method_;
      http_incoming_stream & incoming_stream_;
    };
  private:
    http_incoming_stream & incoming_stream_;
  };
}

#endif//__VDS_HTTP_HTTP_STREAM_READER_H_
