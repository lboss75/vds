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
      using base_class = sequence_step<context_type, void(network_socket &)>;
    public:
      handler(
        const context_type & context,
        const socket_server & args)
      : base_class(context),
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
      
      void processed(){
        this->task_.schedule();        
      }
      
    private:
      accept_socket_task<
        typename base_class::next_step_t,
        typename base_class::error_method_t> task_;
    };
  private:
    const service_provider & sp_;
    std::string address_;
    int port_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
