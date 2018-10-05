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

        struct session_state {
          std::mutex session_mutex_;

          bool blocked_;
          std::chrono::steady_clock::time_point update_time_;
          std::shared_ptr<class dht_session> session_;
          const_data_buffer session_key_;

          session_state()
          : blocked_(false) {
          }
        };
        mutable std::shared_mutex sessions_mutex_;
        std::map<network_address, session_state> sessions_;

        std::future<void> continue_read(const service_provider& sp);
      };
    }
  }
}


#endif //__VDS_DHT_NETWORK_UDP_TRANSPORT_H_
