#ifndef __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_INPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "read_socket_task.h"

namespace vds {
  class input_network_stream
  {
  public:
    input_network_stream(const network_socket & s)
      : s_(s)
    {
    }
    
    template <
      typename next_method_type,
      typename error_method_type
      >
    class handler
    {
    public:
      handler(
        next_method_type & next,
        error_method_type & on_error,
        const input_network_stream & args)
      : task_(next, on_error, args.s_)
      {
      }
      
      void operator()() {
        this->task_.schedule();        
      }
      
    private:
      read_socket_task<
        next_method_type,
        error_method_type> task_;
    };
  private:
    const network_socket & s_;
  };
}

#endif // __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
