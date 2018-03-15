#ifndef __VDS_DHT_NETWORK_UDP_TRANSPORT_H_
#define __VDS_DHT_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "udp_socket.h"

namespace vds {
  namespace dht {
    namespace network {
      class udp_transport : public std::enable_shared_from_this<udp_transport> {
      public:

        void start(
            const service_provider & sp,
            const udp_socket & s);

      private:
        std::map<network_address, std::shared_ptr<dht_session>> sessions_;

        void continue_read(const service_provider &sp, const udp_socket &s);
      };
    }
  }
}


#endif //__VDS_DHT_NETWORK_UDP_TRANSPORT_H_
