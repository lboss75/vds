#ifndef __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class output_network_stream
  {
  public:
    
    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
      >
    class handler
    {
    public:
      handler(
        const done_method_type & done,
        const next_method_type & next,
        const error_method_type & on_error,
        const output_network_stream & args)
      : task_(done, on_error), done_(done)
      {
      }
      
      void operator()(
        const void * data,
        size_t len
      ) const {
        if(0 == len) {
          this->next_();
        }
        else {
          this->task_.set_data(data, len);
          this->task_.schedule(network_manager_);
        }
          
      }
      
      void processed() const
      {
        this->done_();
      }
      
    private:
      const done_method_type & done_;
      network_manager * network_manager_;
      write_socket_task<
        done_method_type,
        error_method_type> task_;
    };
  };
}

#endif // __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
