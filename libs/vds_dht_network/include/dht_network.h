#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_network_client.h"
#include "asymmetriccrypto.h"

namespace vds {
  class server;

  namespace dht {
    namespace network {
      class udp_transport;

      class service {
      public:
        static constexpr uint16_t MIN_HORCRUX = 32;
        static constexpr uint16_t GENERATE_HORCRUX = 64;

        static constexpr uint64_t BLOCK_SIZE = 32ULL * 1024ULL * MIN_HORCRUX;//32K 

        expected<void> register_services(service_registrator& registrator);
        expected<void> start(
          const service_provider * sp,
          const std::shared_ptr<iudp_transport> & udp_transport,
          uint16_t port,
          bool dev_network);
        expected<void> stop();
        vds::async_task<expected<void>> prepare_to_stop();


        static expected<std::list<const_data_buffer>> select_near(
          const database_read_transaction& t,
          const const_data_buffer& target,
          size_t count);


      private:
        client client_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_SERVICE_H_
