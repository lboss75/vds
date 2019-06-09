#ifndef __VDS_DHT_NETWORK_IUDP_TRANSPORT_H_
#define __VDS_DHT_NETWORK_IUDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "asymmetriccrypto.h"

namespace vds {
  class udp_datagram;

  namespace dht {
    namespace network {
      class iudp_transport : public std::enable_shared_from_this<iudp_transport> {
      public:
        virtual async_task<expected<void>> start(
          const service_provider * sp,
          const std::shared_ptr<asymmetric_public_key> & node_cert,
          const std::shared_ptr<asymmetric_private_key> & node_key,
          uint16_t port,
          bool dev_network) = 0;

        virtual void stop() = 0;

        virtual async_task<expected<void>> write_async( const udp_datagram& datagram) = 0;
        virtual async_task<expected<void>> try_handshake( const std::string& address) = 0;
        virtual async_task<expected<void>> on_timer() = 0;

      };
    }
  }
}


#endif //__VDS_DHT_NETWORK_IUDP_TRANSPORT_H_
