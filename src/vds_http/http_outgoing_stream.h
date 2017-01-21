#ifndef __VDS_HTTP_HTTP_OUTGOING_STREAM_H_
#define __VDS_HTTP_HTTP_OUTGOING_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_stream_reader.h"

namespace vds {
  
  class http_outgoing_stream
  {
  public:
    void set_body(const std::string & body)
    {

    }

    const std::string & body() const {
      return this->body_;
    }
    
    size_t size() const {
      return this->body_.size();
    }
    
    template <
      typename next_method_type, 
      typename error_method_type
    >
    void get_reader(
      next_method_type & next_method,
      error_method_type & error_method,
      http_stream_reader<
        next_method_type,
        error_method_type
      > & body_stream)
    {
      body_stream.reset(
        next_method,
        error_method);
    }

    
  private:
    std::string body_;
  };
}

#endif // __HTTP_RESPONSE_STREAM_H_
