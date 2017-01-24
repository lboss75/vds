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
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void(void)>
    {
    public:
      handler(
        context_type & context,
        const output_network_stream & args)
      : base(context),
        task_(*this, on_error),
        s_(args.s_.handle())
      {
      }
      
      void operator()(
        const void * data,
        size_t len
      ) {
        if(0 == len) {
          this->next();
        }
        else {
          this->task_.set_data(data, len);
          this->task_.schedule(this->s_);
        }          
      }

      void processed()
      {
        this->prev.processed();
      }
      
    private:
      network_socket::SOCKET_HANDLE s_;
      write_socket_task<handler, error_method_t> task_;
    };
  private:
    const network_socket & s_;
  };
}

#endif // __VDS_NETWORK_OUTPUT_NETWORK_STREAM_H_
