#ifndef __VDS_DHT_NETWORK_DHT_SESSION_H_
#define __VDS_DHT_NETWORK_DHT_SESSION_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
#include "const_data_buffer.h"
#include "dht_datagram_protocol.h"
#include "udp_transport.h"

namespace vds {
  namespace dht {
    namespace network {

      class dht_session : public dht_datagram_protocol<dht_session, udp_transport> {
        using base_class = dht_datagram_protocol<dht_session, udp_transport>;

      public:
        dht_session(
          const network_address & address,
          const const_data_buffer & this_node_id,
          const const_data_buffer & partner_node_id);

        async_task<> ping_node(
          const service_provider & sp,
          const const_data_buffer & node_id,
          const std::shared_ptr<udp_transport> & transport);

        async_task<> process_message(
            const service_provider & sp,
            uint8_t message_type,
            const const_data_buffer & message);
        const const_data_buffer & partner_node_id() const {
          return this->partner_node_id_;
        }

      private:
        const_data_buffer partner_node_id_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_SESSION_H_
