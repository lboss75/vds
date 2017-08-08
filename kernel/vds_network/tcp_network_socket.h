#ifndef __VDS_NETWORK_NETWORK_SOCKET_H_
#define __VDS_NETWORK_NETWORK_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "dataflow.h"
#include "async_task.h"
#include "async_stream.h"

namespace vds {
  class _tcp_network_socket;
  
  class tcp_network_socket
  {
  public:
    tcp_network_socket();
    ~tcp_network_socket();
    
    static async_task<const tcp_network_socket &> connect(
      const service_provider & sp,
      const std::string & server,
      const uint16_t port);
    
    std::shared_ptr<continuous_stream<uint8_t>> incoming() const;
    std::shared_ptr<continuous_stream<uint8_t>> outgoing() const;
    
    _tcp_network_socket * operator -> () const { return this->impl_.get(); }

    void close();
    
  private:
    friend class _tcp_network_socket;
    tcp_network_socket(const std::shared_ptr<_tcp_network_socket> & impl);
    std::shared_ptr<_tcp_network_socket> impl_;
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_