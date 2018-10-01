#ifndef __VDS_DHT_NETWORK_UDP_TRANSPORT_H_
#define __VDS_DHT_NETWORK_UDP_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "udp_socket.h"
#include "legacy.h"
#include "debug_mutex.h"
#include "iudp_transport.h"

namespace vds {
  struct session_statistic;
}

namespace vds {
  namespace dht {
    namespace network {
      class udp_transport : public iudp_transport {
      public:
        static constexpr uint32_t MAGIC_LABEL = 0xAFAFAFAF;
        static constexpr uint8_t PROTOCOL_VERSION = 0;

        udp_transport();
        udp_transport(const udp_transport&) = delete;
        udp_transport(udp_transport&&) = delete;

        void start(
          const service_provider& sp,
          const std::shared_ptr<certificate> & node_cert,
          const std::shared_ptr<asymmetric_private_key> & node_key,
          uint16_t port) override;

        void stop(const service_provider& sp) override;

        std::future<void> write_async(const service_provider& sp, const udp_datagram& datagram);
        std::future<void> try_handshake(const service_provider& sp, const std::string& address);

        const const_data_buffer& this_node_id() const {
          return this->this_node_id_;
        }

        void get_session_statistics(session_statistic& session_statistic);

      private:
        const_data_buffer this_node_id_;
        std::shared_ptr<certificate> node_cert_;
        std::shared_ptr<asymmetric_private_key> node_key_;
        udp_server server_;

        std::shared_ptr<vds::udp_datagram_reader> reader_;
        std::shared_ptr<vds::udp_datagram_writer> writer_;

#ifdef _DEBUG
#ifndef _WIN32
        pid_t owner_id_;
#else
        DWORD owner_id_;
#endif//_WIN32
#endif//_DEBUG


        mutable std::shared_mutex sessions_mutex_;
        std::map<network_address, std::shared_ptr<class dht_session>> sessions_;

        std::mutex block_list_mutex_;
        std::map<std::string, std::chrono::steady_clock::time_point> block_list_;

        void add_session(const service_provider& sp, const network_address& address,
                         const std::shared_ptr<dht_session>& session);
        std::shared_ptr<dht_session> get_session(const network_address& address) const;

        std::future<void> continue_read(const service_provider& sp);
      };
    }
  }
}


#endif //__VDS_DHT_NETWORK_UDP_TRANSPORT_H_
