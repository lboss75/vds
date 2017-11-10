#ifndef __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_
#define __VDS_P2P_NETWORK_P2P_TRANSPORT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <queue>

#include "async_task.h"
#include "udp_socket.h"

namespace vds {

  class _udp_transport : public std::enable_shared_from_this<_udp_transport> {
  public:
    _udp_transport(const udp_socket & socket);
    ~_udp_transport();   


    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    udp_socket socket_;
    udp_datagram incomming_buffer_;
    bool incomming_eof_;

    struct address_t {
      std::string server_;
      uint16_t port_;

      address_t(
          const std::string & server,
          uint16_t port)
          : server_(server), port_(port)
      {
      }

      bool operator < (const address_t & other) const {
        return (this->server_ < other.server_) ? true :
          ((this->port_ < other.port_) ? true : false);
      }
    };

    class session : public std::enable_shared_from_this<session> {
    public:


      const address_t & address() const {
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

      async_task<> incomming_message(
          const service_provider & sp,
          const uint8_t * data,
          uint16_t size);

      void on_timer();

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
      enum class control_type : uint8_t {
        Handshake =       0b1000,//seq: version
        Keep_alive =      0b1001,//
        Acknowledgement = 0b1010,//seq: last package
        Failed          = 0b1011 //seq: last package, info: failed bits
      };

      address_t address_;
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

      std::mutex incoming_sequence_mutex_;
      uint32_t min_incoming_sequence_;
      std::map<uint32_t, const_data_buffer> future_data_;

      uint16_t expected_size_;
      const_data_buffer expected_buffer_;
      uint8_t  * expected_buffer_p_;


      void send(const service_provider & sp, const const_data_buffer & data);
      void on_timeout();

      const_data_buffer next_message();

      void continue_read_outgoing_stream(const service_provider & sp);

      async_task<> push_data(
          const service_provider & sp,
          const const_data_buffer & data);

      async_task<> continue_process_incoming_data(
          const service_provider & sp);

    };

    std::map<address_t, std::shared_ptr<session>> sessions_;
    std::mutex sessions_mutex_;

    class datagram_generator {
    public:
      datagram_generator(const std::shared_ptr<session> & owner)
      : owner_(owner) {
      }

      virtual ~datagram_generator(){}

      virtual uint16_t generate_message(
          uint8_t * buffer) = 0;

      virtual bool is_eof() const = 0;

      const std::shared_ptr<session> & owner(){
        return this->owner_;
      }

    private:
      std::shared_ptr<session> owner_;
    };

    class data_datagram : public datagram_generator {
    public:
      data_datagram(
          const std::shared_ptr<session> & owner,
          const const_data_buffer & data )
          : datagram_generator(owner), data_(data), offset_(0)
      {
      }

      virtual uint16_t generate_message(
          uint8_t * buffer) override{

        auto seq_number = this->owner()->output_sequence_number();
        ((uint32_t *)buffer)[0] = htonl(seq_number);

        if(0 == this->offset_){
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |0|                     Seq. No.                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |        Size                 |                                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                                 +
    |                                                               |
    ~                            Data                               ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
          ((uint16_t *)buffer)[3] = htons(this->data_.size());
          auto size = this->owner()->mtu() - 6;
          if(size > this->data_.size()){
            size = this->data_.size();
          }

          memcpy(buffer + 6, this->data_.data(), size);
          this->offset_ += size;
          return size + 6;

        } else {
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |0|                     Seq. No.                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    ~                            Data                               ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
          auto size = this->owner()->mtu() - 4;
          if(size > this->data_.size() - this->offset_){
            size = this->data_.size() - this->offset_;
          }

          memcpy(buffer + 4, this->data_.data() + this->offset_, size);
          this->offset_ += size;
          return size + 4;
        }
      }

      bool is_eof() const override {
        return (this->offset_ >= this->data_.size());
      }

    private:
      const_data_buffer data_;
      uint16_t  offset_;
    };

    class acknowledgement_datagram : public datagram_generator {
    public:
      acknowledgement_datagram(
          const std::shared_ptr<session> &owner,
          const uint16_t min_sequence,
          const uint16_t max_sequence)
          : datagram_generator(owner),
            min_sequence_(min_sequence),
            max_sequence_(max_sequence_) {
      }

      virtual uint16_t generate_message(
          uint8_t *buffer) override {

      }
      bool is_eof() const override {
        return true;
      }

    private:
       uint16_t min_sequence_;
       uint16_t max_sequence_;
    };
    static constexpr uint16_t max_datagram_size = 65507;

    uint8_t buffer_[max_datagram_size];

    std::queue<std::unique_ptr<datagram_generator>> send_data_buffer_;
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
