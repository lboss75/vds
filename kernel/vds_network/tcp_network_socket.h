#ifndef __VDS_NETWORK_NETWORK_SOCKET_H_
#define __VDS_NETWORK_NETWORK_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>

#include "async_buffer.h"
#include "network_address.h"
#include "socket_base.h"

namespace vds {

  class tcp_network_socket
#ifdef _WIN32
      : public std::enable_shared_from_this<tcp_network_socket>
#else
      : public socket_base
#endif//_WIN32
  {
  public:
    //tcp_network_socket();
    ~tcp_network_socket();

    static expected<std::shared_ptr<tcp_network_socket>> connect(
      const service_provider * sp,
      const network_address & address);

    expected<
    std::tuple<
      std::shared_ptr<vds::stream_input_async<uint8_t>>,
      std::shared_ptr<vds::stream_output_async<uint8_t>>>> start(
        const service_provider * sp);

    expected<void> close();

    class _tcp_network_socket * operator ->() const {
      return this->impl_;
    }

    bool operator ! () const;

#ifndef _WIN32
        expected<void> process(uint32_t events) override;
#endif

  private:
    friend class _tcp_network_socket;
    tcp_network_socket(class _tcp_network_socket * impl);

    _tcp_network_socket * impl_;
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_
