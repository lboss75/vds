#ifndef __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "write_socket_task.h"

namespace vds {
  class output_network_stream
  {
  public:
    output_network_stream(const network_socket & s)
      : s_(s)
    {
    }
    
    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
      >
    class handler
    {
    public:
      handler(
        done_method_type & done,
        next_method_type & next,
        error_method_type & on_error,
        const output_network_stream & args)
      : task_(done, on_error, args.s_),
        done_(done), next_(next)
      {
      }
      
      void operator()(
        const void * data,
        size_t len
      ) {
        if(0 == len) {
          this->next_();
        }
        else {
          this->task_.set_data(data, len);
          this->task_.schedule();
        }
          
      }
      
      void processed()
      {
        this->done_();
      }
      
    private:
      done_method_type & done_;
      next_method_type & next_;
      write_socket_task<
        done_method_type,
        error_method_type> task_;
    };
  private:
    const network_socket & s_;
  };
}

#endif // __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
