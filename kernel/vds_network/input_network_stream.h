#ifndef __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
#define __VDS_NETWORK_INPUT_NETWORK_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "read_socket_task.h"
#include "pipeline_filter.h"
#include "network_manager.h"

namespace vds {
  class input_network_stream
  {
  public:
    input_network_stream(
      const service_provider & sp,
      const network_socket & s)
      : sp_(sp), s_(s)
    {
    }
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void * data, size_t len)>
    {
      using base = sequence_step<context_type, void(const void * data, size_t len)>;
    public:
      handler(
        const context_type & context,
        const input_network_stream & args)
      : base(context),
        task_(args.sp_, this->next, this->error, args.s_)
      {
      }
      
      void operator()() {
        this->processed();        
      }

      void processed() {
        this->task_();
      }
      
    private:
      read_socket_task<
        typename base::next_step_t,
        typename base::error_method_t> task_;
    };
  private:
    service_provider sp_;
    const network_socket & s_;
  };
}

#endif // __VDS_NETWORK_INPUT_NETWORK_STREAM_H_
