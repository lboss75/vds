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

      class dht_session : public dht_datagram_protocol {
        using base_class = dht_datagram_protocol;

      public:
        dht_session(
          const service_provider * sp,
          const network_address& address,
          const const_data_buffer& this_node_id,
          asymmetric_public_key partner_node_key,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key) noexcept;

        vds::async_task<vds::expected<void>> ping_node(
          const const_data_buffer& node_id,
          const std::shared_ptr<iudp_transport>& transport);

        vds::async_task<vds::expected<bool>> process_message(
          const std::shared_ptr<iudp_transport>& transport,
          uint8_t message_type,
          const const_data_buffer & target_node,
          const std::vector<const_data_buffer> & hops,
          const const_data_buffer& message);

        session_statistic::session_info get_statistic() const;

      private:
        uint64_t time_shift_ = 0;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_SESSION_H_
