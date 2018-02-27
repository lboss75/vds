#ifndef __VDS_P2P_NETWORK_UDP_TRANSPORT_QUEUE_H_
#define __VDS_P2P_NETWORK_UDP_TRANSPORT_QUEUE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <queue>
#include <state_machine.h>
#include "udp_transport_session_p.h"
#include "leak_detect.h"

namespace vds {
  class _udp_transport_queue : public std::enable_shared_from_this<_udp_transport_queue> {
  public:
    _udp_transport_queue();

    void send_data(
        const service_provider & sp,
        const std::shared_ptr<_udp_transport> & owner,
        const std::shared_ptr<_p2p_crypto_tunnel> & session,
        const const_data_buffer & data);

    void stop(const service_provider & sp);

  private:
    friend class _udp_transport_session;
    friend class _udp_transport;

    class datagram_generator {
    public:
      datagram_generator(const std::shared_ptr<_p2p_crypto_tunnel> & owner)
      : owner_(owner) {
      }

      virtual ~datagram_generator() {
      }

      virtual uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) = 0;

      virtual  void complete(
          const uint8_t * buffer, size_t len) = 0;

      virtual bool is_eof() const = 0;

      const std::shared_ptr<_p2p_crypto_tunnel> & owner() const {
        return this->owner_;
      }

    private:
      std::shared_ptr<_p2p_crypto_tunnel> owner_;
    };

    class data_datagram : public datagram_generator {
    public:
      data_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> & owner,
          const const_data_buffer & data )
          : datagram_generator(owner),
            data_(data), offset_(0) {
      }

      //Generate message
      virtual uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) override;

      //Store sent message
      void complete(const uint8_t * buffer, size_t len) override;

      //
      bool is_eof() const override {
        return (this->offset_ >= this->data_.size());
      }

    private:
      const_data_buffer data_;
      uint16_t  offset_;
    };

    class repeat_datagram : public datagram_generator {
    public:
      repeat_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> & owner,
          const uint32_t sequence_number)
          : datagram_generator(owner), sequence_number_(sequence_number) {
      }

      //Generate message
      virtual uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) override;

      //Store sent message
      void complete(const uint8_t * buffer, size_t len) override {
      }

      //
      bool is_eof() const override {
        return true;
      }

    private:
      uint32_t  sequence_number_;
    };

    class handshake_datagram : public datagram_generator {
    public:
      handshake_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> & owner,
          const guid & instance_id)
          : datagram_generator(owner),
          instance_id_(instance_id) {
      }

      virtual uint16_t generate_message(const service_provider &sp, uint8_t *buffer) override;

      void complete(const uint8_t * buffer, size_t len) override;

      bool is_eof() const override {
        return true;
      }

    private:
      guid instance_id_;
    };

    class welcome_datagram : public datagram_generator {
    public:
      welcome_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> & owner,
          const guid & instance_id)
          : datagram_generator(owner),
            instance_id_(instance_id) {
      }

      virtual uint16_t generate_message(const service_provider &sp, uint8_t *buffer) override;

      void complete(const uint8_t * buffer, size_t len) override;

      bool is_eof() const override {
        return true;
      }

    private:
      guid instance_id_;
    };

    class keep_alive_datagram : public datagram_generator {
    public:
      keep_alive_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> &owner)
      : datagram_generator(owner) {
      }

      virtual uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) override;

      bool is_eof() const override {
        return true;
      }

      void complete(const uint8_t * buffer, size_t len) override {
      }
    };

    class acknowledgement_datagram : public datagram_generator {
    public:
      acknowledgement_datagram(
          const std::shared_ptr<_p2p_crypto_tunnel> &owner)
          : datagram_generator(owner) {
      }

      virtual uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) override;

      bool is_eof() const override {
        return true;
      }

      void complete(const uint8_t * buffer, size_t len) override {
      }

    };

    class failed_datagram : public datagram_generator {
    public:
      failed_datagram(const std::shared_ptr<_p2p_crypto_tunnel> &owner)
          : datagram_generator(owner) {
      }

      //Generate message
      uint16_t generate_message(
          const service_provider &sp,
          uint8_t *buffer) override;

      //Store sent message
      void complete(const uint8_t * buffer, size_t len) override {
      }

      //
      bool is_eof() const override {
        return true;
      }
    };

    std::list<std::shared_ptr<datagram_generator>> send_data_buffer_;
    std::mutex send_data_buffer_mutex_;

    enum state_t
    {
      bof,
      start_write,
      write_scheduled,
      write_pending,
      close_pending,
      closed,
      failed
    };
    state_machine<state_t> current_state_;

    static constexpr uint16_t max_datagram_size = 65507;
    uint8_t buffer_[max_datagram_size];

    void continue_send_data(
        const service_provider & sp,
        const std::shared_ptr<_udp_transport> & owner);

    void emplace(
        const service_provider & sp,
        const std::shared_ptr<_udp_transport> & owner,
        datagram_generator * item);
  };
}

#endif //__VDS_P2P_NETWORK_UDP_TRANSPORT_QUEUE_H_
