#ifndef __VDS_HTTP_HTTP_JSON_FORMATTER_H_
#define __VDS_HTTP_HTTP_JSON_FORMATTER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "http_response.h"
#include "http_outgoing_stream.h"

namespace vds {
  class http_json_formatter
  {
  public:
    http_json_formatter(
      http_response & response,
      http_outgoing_stream & response_stream
    ) : response_(response),
        response_stream_(response_stream)
    {
    }
    
    template <typename context_type>
    class handler : public dataflow_step<context_type, void(
      http_response & response,
      http_outgoing_stream & response_stream)>
    {
      using base_class = dataflow_step<context_type, void(
        http_response & response,
        http_outgoing_stream & response_stream)>;
    public:
      handler(
        const context_type & context,
        const http_json_formatter & args
      ) : base_class(context),
          response_(args.response_),
          response_stream_(args.response_stream_)
      {
      }
      
      void operator()(const service_provider & sp, json_value * value)
      {
        if(nullptr != value){
          this->response_.set_result(
            http_response::HTTP_OK,
            "OK"
          );
          this->response_.add_header("Content-Type", "application/json");
          
          this->response_stream_.set_body(value->str());
        }
        else {
          this->response_.clear();
        }
        this->next(
          sp,
          this->response_,
          this->response_stream_);
      }
            
    private:
      http_response & response_;
      http_outgoing_stream & response_stream_;
    };
  private:
    http_response & response_;
    http_outgoing_stream & response_stream_;
  };
}

#endif // HTTP_JSON_FORMATTER_H
