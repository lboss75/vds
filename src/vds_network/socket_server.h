#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

namespace vds {
  class socket_server
  {
  public:
    
    template <
      typename next_method_type,
      typename error_method_type
      >
    class source
    {
    public:
      source(
        next_method_type next,
        error_method_type on_error,
        const socket_server & args)
      : task_(next, on_error)
      {
      }
      
      void operator()() {
        this->task_.schedule(network_manager_);        
      }
      
    private:
      network_manager * network_manager_;
      accept_socket_task<next_method_type, error_method_type> task_;
    };
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
