#ifndef __VDS_DHT_NETWORK_UDP_TRANSPORT_H_
#define __VDS_DHT_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "udp_socket.h"
#include "legacy.h"

namespace vds {
  namespace dht {
    namespace network {
      class udp_transport : public std::enable_shared_from_this<udp_transport> {
      public:
        static constexpr size_t NODE_ID_SIZE = 32;
        static constexpr uint8_t PROTOCOL_VERSION = 0;

        udp_transport();
        udp_transport(const udp_transport &) = delete;
        udp_transport(udp_transport &&) = delete;

        void start(const vds::service_provider &sp, uint16_t port,
                           const const_data_buffer &this_node_id);

        void stop(const service_provider & sp);

        async_task<> write_async(const udp_datagram & datagram);
        void on_timer(const service_provider& sp);

      private:
        const_data_buffer this_node_id_;
        udp_server server_;

        std::mutex write_mutex_;
        std::condition_variable write_cond_;
        bool write_in_progress_;

        mutable std::shared_mutex sessions_mutex_;
        std::map<network_address, std::shared_ptr<class dht_session>> sessions_;

        void add_session(const service_provider& sp, const network_address & address, const std::shared_ptr<dht_session> & session);
        std::shared_ptr<dht_session> get_session(const network_address & address) const;

        void continue_read(const service_provider &sp);
      };
    }
  }
}


#endif //__VDS_DHT_NETWORK_UDP_TRANSPORT_H_
