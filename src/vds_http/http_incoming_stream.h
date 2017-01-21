#ifndef __VDS_HTTP_HTTP_INCOMING_STREAM_H_
#define __VDS_HTTP_HTTP_INCOMING_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"

namespace vds {
  
  class http_incoming_stream
  {
  public:
    
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
  };
}

#endif // __VDS_HTTP_HTTP_INCOMING_STREAM_H_
