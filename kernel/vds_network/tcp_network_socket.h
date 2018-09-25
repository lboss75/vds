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
      const network_address & address);

    std::shared_ptr<vds::input_stream_async<uint8_t>> start(
        const service_provider & sp) const;

    void close();

    class _tcp_network_socket * operator ->() const;

  private:
    friend class _tcp_network_socket;
    tcp_network_socket(class _tcp_network_socket * impl);
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_
