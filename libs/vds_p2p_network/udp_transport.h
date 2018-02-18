#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "async_buffer.h"
#include "leak_detect.h"
#include "leak_detect.h"

namespace vds {

  class udp_transport {
  public:
    static constexpr uint32_t protocol_version = 0;

    udp_transport();
	  ~udp_transport();

    void start(
        const vds::service_provider &sp,
        int port);

    void stop(const service_provider & sp);

    void connect(
        const service_provider & sp,
        const std::string & address);

    async_task<> prepare_to_stop(const service_provider &sp);

    class _udp_transport *operator -> () const {
      return this->impl_.get();
    }
    operator bool () const {
      return nullptr != this->impl_.get();
    }
  private:
    std::shared_ptr<class _udp_transport> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_H_
