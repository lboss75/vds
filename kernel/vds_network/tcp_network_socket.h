#ifndef __VDS_NETWORK_NETWORK_SOCKET_H_
#define __VDS_NETWORK_NETWORK_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>

#include "async_buffer.h"
#include "network_address.h"

namespace vds {

  class tcp_network_socket : public stream_async<uint8_t>
  {
  public:
    tcp_network_socket();

    static tcp_network_socket connect(
      const service_provider & sp,
      const network_address & address,
      const stream<uint8_t> & input_handler);
    
    void start(
        const service_provider & sp,
        const stream<uint8_t> & input_handler) const;

    void close();

    class _tcp_network_socket * operator ->() const;

  private:
    friend class _tcp_network_socket;
    tcp_network_socket(class _tcp_network_socket * impl);
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_
