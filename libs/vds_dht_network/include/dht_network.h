#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_network_client.h"

namespace vds {
  namespace dht {
    namespace network {
      class udp_transport;

      class service {
      public:
	      void register_services(service_registrator & registrator);
	      void start(const service_provider &sp, uint16_t port);
	      void stop(const service_provider &);
	      async_task<> prepare_to_stop(const service_provider &sp);

      private:
				client client_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_
