#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "async_buffer.h"

namespace vds {

  class udp_transport {
  public:
    class session {
    public:
      const std::string & address() const;

      async_task<> send(
          const service_provider & sp,
          const const_data_buffer & message);

      class _session * operator -> () const { return this->impl_.get(); }

    private:
      std::shared_ptr<class _session> impl_;
    };

    typedef std::function<void(const session & source, const const_data_buffer & message)> message_handler_t;

    udp_transport(const message_handler_t & message_handler);

    void start(const vds::service_provider &sp, int port);
    void stop(const service_provider & sp);

    void connect(
        const service_provider & sp,
        const std::string & address);

  private:
    std::shared_ptr<class _udp_transport> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_H_
