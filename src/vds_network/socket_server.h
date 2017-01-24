#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "accept_socket_task.h"

namespace vds {
  class service_provider;
  
  class socket_server
  {
  public:
    socket_server(
      const service_provider & sp,
      const std::string & address,
      int port)
    : sp_(sp), address_(address), port_(port)
    {
    }
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void(network_socket &)>
    {
    public:
      handler(context_type & context,
        const socket_server & args)
      : base(context),
        task_(
          this->next,
          this->error,
          args.sp_,
          args.address_,
          args.port_)
      {
      }
      
      void operator()() {
        this->task_.schedule();        
      }
      
    private:
      accept_socket_task<next_step_t, error_method_t> task_;
    };
  private:
    const service_provider & sp_;
    std::string address_;
    int port_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
