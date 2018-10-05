#ifndef __VDS_DHT_NETWORK_DHT_SESSION_H_
#define __VDS_DHT_NETWORK_DHT_SESSION_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "const_data_buffer.h"
#include "dht_datagram_protocol.h"
#include "udp_transport.h"
#include "session_statistic.h"

namespace vds {
  namespace dht {
    namespace network {

      class dht_session : public dht_datagram_protocol<dht_session, iudp_transport> {
        using base_class = dht_datagram_protocol<dht_session, iudp_transport>;

      public:
        dht_session(
          const network_address& address,
          const const_data_buffer& this_node_id,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key);

        std::future<void> ping_node(
          const service_provider& sp,
          const const_data_buffer& node_id,
          const std::shared_ptr<iudp_transport>& transport);

        std::future<void> process_message(
          const service_provider& sp,
          const std::shared_ptr<iudp_transport>& transport,
          uint8_t message_type,
          const const_data_buffer & target_node,
          const const_data_buffer & source_node,
          uint16_t hops,
          const const_data_buffer& message);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_SESSION_H_
