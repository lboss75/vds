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
      class handler : public dataflow_step<context_type, bool(const std::string &)>
    {
      using base_class = dataflow_step<context_type, bool(const std::string &)>;
    public:
      handler(
        const context_type & context,
        const http_simple_response_reader & args)
        : base_class(context)
      {
      }
      
      ~handler()
      {
      }

      bool operator()(
        const service_provider & sp,
        http_response * response,
        http_incoming_stream * response_stream
        )
      {
        if(nullptr == response){
          return this->next(sp, std::string());
        }
        
        if(http_response::HTTP_OK == response->code()){
          dataflow(
            http_stream_reader<typename base_class::prev_step_t>(
              this->prev,
              *response_stream),
            stream_to_string()
          )
          (
            [this](const service_provider & sp, const std::string & value){
              if(this->next(sp, value)){
                this->prev(sp);
              }
            },
           this->error,
           sp
          );
          
          return false;
        }
        return true;
      }
    private:
    };    
  };
}

#endif // __VDS_HTTP_HTTP_SIMPLE_RESPONSE_READER_H_
