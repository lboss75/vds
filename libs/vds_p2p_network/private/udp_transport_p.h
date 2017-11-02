#ifndef __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_
#define __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <unordered_map>
#include <queue>

namespace vds {

  class _udp_transport : public std::enable_shared_from_this<_udp_transport> {
  public:
    _udp_transport(const udp_socket & socket);
    ~_udp_transport();

    const stream_async<const_data_buffer> & outgoing_stream();
    const stream_async<const_data_buffer> & incoming_network_stream();

    void start(const service_provider & sp);

  private:
    udp_socket socket_;
    udp_datagram incomming_buffer_;
    bool incomming_eof_;

    struct address {
      std::string server_;
      uint16_t port_;

      address(
          const std::string & server,
          uint16_t port)
          : server_(server), port_(port)
      {
      }
    };

    class session : public std::enable_shared_from_this<session> {
    public:

      const address & address() const {
        return this->address_;
      }

      uint32_t output_sequence_number() const {
        return this->output_sequence_number_;
      }

      uint16_t mtu() const {
        return this->mtu_;
      }

      void mtu(uint16_t value) {
        this->mtu_ = value;
      }

      void decrease_mtu();

      void message_sent();

    private:
      enum class send_state{
        bof,
        wait_message
      };

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |1|type |           ACK Seq. No.                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    ~                 Control Information Field                     ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
      enum class control_type : public uint32_t {
        Handshake =       1000b,//seq: version, info: node guid, mtu size
        Keep_alive =      1001b,//
        Acknowledgement = 1010b,//seq: last package
        Failed          = 1011b,//seq: last package, info: failed bits
      };

      address address_;
      uint32_t output_sequence_number_;
      uint32_t input_sequence_number_;

      timer timeout_timer_;
      guid node_id_;
      uint16_t mtu_;
      send_state current_state_;

      enum class outgoing_stream_state {
        ready_to_send,
        eof

      };

      continuous_buffer<const_data_buffer> outgoing_stream_;
      const_data_buffer outgoing_stream_buffer_[16];
      size_t outgoing_stream_buffer_count_;

      outgoing_stream_state outgoing_stream_state_;
      std::mutex outgoing_stream_state_mutex_;


      const_data_buffer outgoing_network_datagrams_[16];
      int outgoing_network_count_;

      continuous_buffer<const_data_buffer> incoming_network_stream_;

      enum class send_data_state {

      };
      send_data_state send_data_state_;
      std::mutex send_data_state_mutex_;
      udp_datagram send_data_;


      void send(const service_provider & sp, const const_data_buffer & data);
      void on_timeout();

      const_data_buffer next_message();

      void continue_read_outgoing_stream(const service_provider & sp);
    };

    std::unordered_map<address, std::shared_ptr<session>> sessions_;
    std::mutex sessions_mutex_;

    struct datagram{
      std::shared_ptr<session> owner_;
      const_data_buffer data_;
      uint16_t  offset_;
    };

    static constexpr uint16_t max_datagram_size = 65507;

    uint8_t buffer_[max_datagram_size];

    std::queue<datagram> send_data_buffer_;
    std::mutex send_data_buffer_mutex_;

    void continue_read_socket(const service_provider & sp);

    void continue_send_data(const service_provider & sp);
    void send_data(
        const service_provider & sp,
        const std::shared_ptr<session> & session,
        const const_data_buffer & data);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_
