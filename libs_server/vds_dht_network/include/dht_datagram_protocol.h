#ifndef __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
#define __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <queue>

#include "udp_socket.h"
#include "const_data_buffer.h"
#include "udp_datagram_size_exception.h"
#include "vds_debug.h"
#include "messages/dht_route_messages.h"
#include "network_address.h"
#include "debug_mutex.h"
#include "vds_exceptions.h"
#include "hash.h"
#include "session_statistic.h"
#include "asymmetriccrypto.h"

namespace vds {
  namespace dht {
    namespace network {
      class iudp_transport;

      class dht_datagram_protocol : public std::enable_shared_from_this<dht_datagram_protocol> {
      public:
        static constexpr int CHECK_MTU_TIMEOUT = 10;
        static constexpr int MIN_MTU = 508;

        dht_datagram_protocol(
          const service_provider * sp,
          const network_address& address,
          const const_data_buffer& this_node_id,
          asymmetric_public_key partner_node_key,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key) noexcept;

        virtual ~dht_datagram_protocol();

        void set_mtu(uint16_t value);

        vds::async_task<vds::expected<void>> send_message(
          const std::shared_ptr<iudp_transport>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          expected<const_data_buffer>&& message);

        vds::async_task<vds::expected<void>> send_message(
          const std::shared_ptr<iudp_transport>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const const_data_buffer& message);

        vds::async_task<vds::expected<void>> proxy_message(
          std::shared_ptr<iudp_transport> s,
          uint8_t message_type,
          const_data_buffer target_node,
          std::vector<const_data_buffer> hops,
          const_data_buffer message);

        vds::async_task<vds::expected<void>> process_datagram(
          const std::shared_ptr<iudp_transport>& s,
          const_data_buffer datagram);

        const network_address& address() const {
          return this->address_;
        }

        const const_data_buffer& this_node_id() const {
          return this->this_node_id_;
        }

        const const_data_buffer& partner_node_id() const {
          return this->partner_node_id_;
        }

        const asymmetric_public_key & partner_node_key() const {
          return this->partner_node_key_;
        }

        vds::async_task<vds::expected<void>> on_timer(
          const std::shared_ptr<iudp_transport>& s);

        void process_pong(
          const const_data_buffer& source_node,
          uint64_t timeout);

      protected:
        const service_provider * sp_;

        std::mutex metrics_mutex_;
        time_t last_metric_;
        std::list<session_statistic::time_metric> metrics_;

        std::mutex traffic_mutex_;
        std::map<const_data_buffer /*from*/, std::map<const_data_buffer /*to*/, std::map<uint8_t /*message_type*/, session_statistic::traffic_info /*size*/>>> traffic_;

        async_task<vds::expected<void>> send_acknowledgment(
          const std::shared_ptr<iudp_transport>& s);

        virtual vds::async_task<vds::expected<bool>> process_message(
          const std::shared_ptr<iudp_transport>& transport,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const std::vector<const_data_buffer>& hops,
          const const_data_buffer& message) = 0;


      private:
        static constexpr uint8_t INDEX_SIZE = 4;
        static constexpr uint8_t SIZE_SIZE = 4;

        static constexpr int SEND_TIMEOUT = 600;

        struct send_queue_item_t {
          std::shared_ptr<iudp_transport> transport;
          uint8_t message_type;
          const_data_buffer target_node;
          const_data_buffer source_node;
          uint16_t hops;
          const_data_buffer message;
        };
        bool failed_state_;
        int check_mtu_;
        int last_sent_;
        network_address address_;
        const_data_buffer this_node_id_;

        asymmetric_public_key partner_node_key_;
        const_data_buffer partner_node_id_;
        const_data_buffer session_key_;

        uint16_t mtu_;
        uint16_t input_mtu_;

        std::mutex output_mutex_;
        uint32_t last_output_index_;
        struct output_message {
          std::chrono::high_resolution_clock::time_point queue_time_;
          const_data_buffer data_;
        };
        std::map<uint32_t, output_message> output_messages_;

        std::mutex input_mutex_;
        uint32_t last_input_index_;
        uint32_t expected_index_;
        std::chrono::steady_clock::time_point last_processed_;
        std::map<uint32_t, const_data_buffer> input_messages_;

        std::atomic<uint64_t> idle_time_ = 0;
        std::atomic<size_t> idle_count_ = 0;

        std::atomic<size_t> delay_ = 0;
        std::atomic<size_t> service_traffic_ = 0;

        vds::async_task<vds::expected<void>> send_message_async(
          std::shared_ptr<iudp_transport> s,
          uint8_t message_type,
          const_data_buffer target_node,
          std::vector<const_data_buffer> hops,
          const_data_buffer message);

        vds::expected<std::tuple<uint32_t, uint32_t>> prepare_to_send(
          uint8_t message_type,
          const_data_buffer target_node,
          std::vector<const_data_buffer> hops,
          const_data_buffer message);

        vds::async_task<vds::expected<void>> continue_process_messages(
          const std::shared_ptr<iudp_transport>& s);

        vds::async_task<vds::expected<void>> process_acknowledgment(
          const std::shared_ptr<iudp_transport>& s,
          const const_data_buffer& datagram);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
