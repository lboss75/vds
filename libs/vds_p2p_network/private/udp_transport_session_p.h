#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <queue>
#include <map>

#include "async_task.h"
#include "udp_socket.h"
#include "resizable_data_buffer.h"
#include "udp_transport.h"

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

  class _udp_transport_session : public udp_transport::_session {
  public:
    _udp_transport_session(
        const guid &instance_id,
        const std::shared_ptr<_udp_transport> & owner,
        const _udp_transport_session_address_t &address)
        : instance_id_(instance_id),
          address_(address),
          output_sequence_number_(0),
          mtu_(65507),
          owner_(owner),
          current_state_(send_state::bof){
    }

    async_task<const const_data_buffer &> read_async(const service_provider &sp) override;

    //Fake session
    _udp_transport_session(const _udp_transport_session_address_t &address)
        : address_(address),
          output_sequence_number_(0),
          mtu_(65507),
          current_state_(send_state::bof){
    }

    ~_udp_transport_session();

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

    uint32_t report_incoming_sequence(uint32_t & result_mask) const {
      std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);

      auto curent = this->min_incoming_sequence_;
      uint32_t result = 0;
      for(int i = 0; i < 8 * sizeof(result); ++i){
        ++curent;
        result <<= 1;
        if(this->future_data_.end() != this->future_data_.find(curent)){
          result |= 1;
        }
      }
      result_mask = result;

      return this->min_incoming_sequence_;
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
      std::unique_lock<std::mutex> lock(this->output_sequence_mutex_);
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

    void send(const service_provider &sp, const const_data_buffer &message) override;

    uint16_t get_sent_data(uint8_t *buffer, uint32_t sequence_number);

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

    std::mutex output_sequence_mutex_;
    uint32_t output_sequence_number_;
    std::map<uint32_t, const_data_buffer> sent_data_;

    uint16_t mtu_;

    send_state current_state_;
    std::mutex state_mutex_;

    std::mutex incoming_sequence_mutex_;
    uint32_t min_incoming_sequence_;
    std::map<uint32_t, const_data_buffer> future_data_;

    uint16_t expected_size_;
    resizable_data_buffer expected_buffer_;

    std::mutex read_result_mutex_;
    async_result<const const_data_buffer &> read_result_;

    std::weak_ptr<_udp_transport> owner_;

    void on_timeout();

    const_data_buffer next_message();

    void continue_read_outgoing_stream(const service_provider & sp);

    async_task<> push_data(
        const service_provider & sp,
        const const_data_buffer & data);

    void continue_process_incoming_data(
        const service_provider &sp,
        class _udp_transport &owner);

    void try_read_data();

    std::mutex incoming_datagram_mutex_;
    std::queue<const_data_buffer> incoming_datagrams_;
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_SESSION_P_H_
