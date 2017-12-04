#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>

#include "async_task.h"
#include "udp_socket.h"
#include "resizable_data_buffer.h"

namespace vds {
  struct _udp_transport_session_address_t {
    std::string server_;
    uint16_t port_;

    _udp_transport_session_address_t(
        const std::string & server,
        uint16_t port)
        : server_(server), port_(port)
    {
    }

    bool operator < (const _udp_transport_session_address_t & other) const {
      return (this->server_ < other.server_) ? true :
             ((this->port_ < other.port_) ? true : false);
    }
  };

  class _udp_transport_session: public std::enable_shared_from_this<_udp_transport_session> {
  public:
    _udp_transport_session(
        const guid &instance_id,
        const _udp_transport_session_address_t &address)
        : instance_id_(instance_id),
          address_(address),
          current_state_(send_state::bof){
    }

    //Fake session
    _udp_transport_session(const _udp_transport_session_address_t &address)
        : address_(address),
          current_state_(send_state::bof){
    }

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
    enum class control_type : uint8_t {
      Handshake = 0b1000,//seq: version, info: node-id
      Welcome = 0b1001,//seq: version, info: node-id
      Keep_alive = 0b1010,//
      Acknowledgement = 0b1011,//seq: last package
      Failed = 0b1100 //seq: last package, info: failed bits
    };

    const _udp_transport_session_address_t &address() const {
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

    void incomming_message(
        const service_provider &sp,
        class _udp_transport &owner,
        const uint8_t *data,
        uint16_t size);

    void on_timer(
        const service_provider & sp,
        const std::shared_ptr<class _udp_transport> &owner);

    void add_datagram(const const_data_buffer &data) {
      this->sent_data_[this->output_sequence_number_++] = data;
    }

    void send_handshake(
        const service_provider &sp,
        const std::shared_ptr<_udp_transport> & owner);

    void handshake_sent();

    void send_welcome(
        const service_provider &sp,
        const std::shared_ptr<_udp_transport> & owner);
    void welcome_sent();

  private:
    enum class send_state {
      bof,
      handshake_pending,
      welcome_pending,
      welcome_sent,
      wait_message
    };

    guid instance_id_;
    _udp_transport_session_address_t address_;
    uint32_t output_sequence_number_;
    uint32_t input_sequence_number_;

    uint16_t mtu_;

    send_state current_state_;
    std::mutex state_mutex_;

    std::map<uint32_t, const_data_buffer> sent_data_;

    enum class outgoing_stream_state {
      ready_to_send,
      eof

    };

    const_data_buffer outgoing_stream_buffer_[16];
    size_t outgoing_stream_buffer_count_;

    outgoing_stream_state outgoing_stream_state_;
    std::mutex outgoing_stream_state_mutex_;


    const_data_buffer outgoing_network_datagrams_[16];
    int outgoing_network_count_;

    enum class send_data_state {

    };

    send_data_state send_data_state_;
    std::mutex send_data_state_mutex_;
    udp_datagram send_data_;

    std::mutex incoming_sequence_mutex_;
    uint32_t min_incoming_sequence_;
    std::map<uint32_t, const_data_buffer> future_data_;

    uint16_t expected_size_;
    resizable_data_buffer expected_buffer_;

    void send(const service_provider & sp, const const_data_buffer & data);
    void on_timeout();

    const_data_buffer next_message();

    void continue_read_outgoing_stream(const service_provider & sp);

    async_task<> push_data(
        const service_provider & sp,
        const const_data_buffer & data);

    void continue_process_incoming_data(
        const service_provider &sp,
        class _udp_transport &owner);

    void process_incoming_datagram(
        const vds::service_provider &sp,
        class _udp_transport &owner,
        const uint8_t *data,
        size_t size);
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_
