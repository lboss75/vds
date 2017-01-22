#ifndef __VDS_HTTP_HTTP_STREAM_READER_H_
#define __VDS_HTTP_HTTP_STREAM_READER_H_

#include <vector>
#include "http_incoming_stream.h"

namespace vds {
  template <typename done_method_type>
  class http_stream_reader
  {
  public:
    http_stream_reader(
      done_method_type & done_method,
      http_incoming_stream & incoming_stream
    )
    : done_method_(done_method_),
    incoming_stream_(incoming_stream)
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
        incoming_stream_(args.incoming_stream_),
        done_method_(args.done_method_)
      {
        this->incoming_stream_.handler(this);
      }
      
      ~handler()
      {
        this->incoming_stream_.handler(nullptr);
        std::cout << "http_stream_reader::~handler\n";
      }
      
      void operator()()
      {
        this->done_method_();
      }
      
      void push_data(
        const void * data,
        size_t len
      ) override
      {
        this->next_method_(data, len);
      }
      
    private:
      next_method_type & next_method_;
      error_method_type & error_method_;
      done_method_type & done_method_;
      http_incoming_stream & incoming_stream_;
    };
  private:
    done_method_type & done_method_;
    http_incoming_stream & incoming_stream_;
  };
}

#endif//__VDS_HTTP_HTTP_STREAM_READER_H_
