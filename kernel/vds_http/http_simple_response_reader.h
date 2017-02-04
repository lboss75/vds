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
      typename context_type
    >
      class handler : public sequence_step<context_type, void(const std::string &)>
    {
      using base_class = sequence_step<context_type, void(const std::string &)>;
    public:
      handler(
        const context_type & context,
        const http_simple_response_reader & args)
        : base_class(context)
      {
      }
      
      ~handler()
      {
        std::cout << "http_simple_response_reader::handler::~handler\n";
      }

      void operator()(
        http_response & response,
        http_incoming_stream & response_stream
        )
      {
        if(http_response::HTTP_OK == response.code()){
          sequence(
            http_stream_reader<typename base_class::prev_step_t>(
              this->prev,
              response_stream),
            stream_to_string()
          )
          (
           this->next,
           this->error
          );
        }
      }
    private:
    };    
  };
}

#endif // __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
