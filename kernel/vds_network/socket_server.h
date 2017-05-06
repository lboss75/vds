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
    class handler : public dataflow_step<context_type, void(network_socket *)>
    {
      using base_class = dataflow_step<context_type, void(network_socket *)>;
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
      
      void operator()(const service_provider & sp) {
        this->task_.schedule(sp);        
      }
      
      void processed(const service_provider & sp){
        this->task_.schedule(sp);        
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
  
  class create_socket_session
  {
  public:
    create_socket_session(
      const std::function<void(const service_provider & sp, network_socket & s)> & create_session)
    : create_session_(create_session)
    {
    }
    
    template <typename context_type>
    class handler : public dataflow_step<context_type, void(void)>
    {
      using base_class = dataflow_step<context_type, void(void)>;
    public:
      handler(
        const context_type & context,
        const create_socket_session & args)
      : base_class(context),
        create_session_(args.create_session_)
      {
      }
      
      void operator()(const service_provider & sp, network_socket * s)
      {
        if(nullptr == s){
          this->next(sp);
        }
        else {
          this->create_session_(sp, *s);
          this->prev(sp);
        }
      }
      
    private:
      std::function<void(const service_provider & sp, network_socket & s)> create_session_;
    };
  private:
    std::function<void(const service_provider & sp, network_socket & s)> create_session_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
