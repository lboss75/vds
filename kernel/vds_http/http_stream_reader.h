#ifndef __VDS_HTTP_HTTP_STREAM_READER_H_
#define __VDS_HTTP_HTTP_STREAM_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
/*
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
    : done_method_(done_method),
    incoming_stream_(incoming_stream)
    {
    }
    
    template<
      typename context_type
    >
    class handler
    : public http_incoming_stream::read_handler,
    public dataflow_step<context_type, bool(
      const void * data,
      size_t len)
    >    
    {
      using base_class = dataflow_step<context_type,
        bool(
          const void * data,
          size_t len)>;
    public:
      handler(
        const context_type & context,
        const http_stream_reader & args
      )
      : base_class(context),
        done_method_(args.done_method_),
        incoming_stream_(args.incoming_stream_)
      {
        this->incoming_stream_.handler(this);
      }
      
      ~handler()
      {
        this->incoming_stream_.handler(nullptr);
      }
      
      bool operator()(const service_provider & sp)
      {
        while(this->done_method_(sp)){
        }
        
        return false;
      }
      
      void push_data(
        const service_provider & sp, 
        const void * data,
        size_t len
      ) override
      {
        this->next(sp, data, len);
      }
      
      void processed(const service_provider & sp)
      {
        while(this->done_method_(sp)){
        }        
      }
      
    private
      done_method_type & done_method_;
      http_incoming_stream & incoming_stream_;
    };
  private
    done_method_type & done_method_;
    http_incoming_stream & incoming_stream_;
  };
}
*/
#endif//__VDS_HTTP_HTTP_STREAM_READER_H_
