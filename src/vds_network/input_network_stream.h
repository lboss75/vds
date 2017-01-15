#ifndef __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_INPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class input_network_stream
  {
  public:
    
    template <
      typename next_method_type,
      typename error_method_type
      >
    class handler
    {
    public:
      handler(
        const next_method_type & next,
        const error_method_type & on_error,
        const input_network_stream & args)
      : task_(next, on_error)
      {
      }
      
      void operator()() const {
        this->task_.schedule(network_manager_);        
      }
      
    private:
      network_manager * network_manager_;
      read_socket_task<
        next_method_type,
        error_method_type> task_;
    };
  };
}

#endif // __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
