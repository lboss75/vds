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

  class tcp_network_socket : public std::enable_shared_from_this<tcp_network_socket>
  {
  public:
    tcp_network_socket();
    ~tcp_network_socket();

    static std::shared_ptr<tcp_network_socket> connect(
      const service_provider & sp,
      const network_address & address);

    std::tuple<
      std::shared_ptr<vds::stream_input_async<uint8_t>>,
      std::shared_ptr<vds::stream_output_async<uint8_t>>> start(
        const service_provider & sp);

    void close();

    class _tcp_network_socket * operator ->() const {
#ifdef _WIN32
      return this->impl_;
#else
      return this->impl_.get();
#endif
    }

  private:
    friend class _tcp_network_socket;
    tcp_network_socket(class _tcp_network_socket * impl);

#ifdef _WIN32
    _tcp_network_socket * impl_;
#else
    std::shared_ptr<_tcp_network_socket> impl_;
#endif
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_
