#include <udp_datagram_size_exception.h>
#include "stdafx.h"
#include "private/udp_transport_queue_p.h"
#include "private/udp_transport_p.h"
#include "vds_debug.h"
#include "private/p2p_crypto_tunnel_p.h"

vds::_udp_transport_queue::_udp_transport_queue()
: current_state_(state_t::bof, state_t::failed) {
}

void vds::_udp_transport_queue::continue_send_data(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {

  if(state_t::closed == this->current_state_.change_state(
      [](state_t & state)->bool{
        switch(state){
          case state_t::start_write:
          case state_t::write_scheduled: {
            state = state_t::write_pending;
            return true;
          }
          case state_t::close_pending: {
            state = state_t::closed;
            return true;
          }
          default:{
            return false;
          }
        }
      },
      error_logic::throw_exception)){
    return;
  }

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto generator = *this->send_data_buffer_.begin();
  this->send_data_buffer_.pop_front()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              ;
  lock.unlock();

  auto len = generator->generate_message(sp, this->buffer_);
  owner->write_async(
          udp_datagram(
              generator->owner()->address(),
              this->buffer_,
              len,
              false))
      .execute(
          [pthis = this->shared_from_this(), sp, owner, generator, len](
              const std::shared_ptr<std::exception> & ex){
            auto owner_ = generator->owner();
            if (ex) {
              auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
              if (datagram_error) {
                owner_->decrease_mtu();
              } else {
                pthis->current_state_.change_state(
                    [](state_t & state) -> bool {
                      state = state_t::closed;
                      return true;
                    }, error_logic::throw_exception);
                owner_->close(sp, ex);
                return;
              }
            } else {
              owner_->register_outgoing_traffic(len);
              generator->complete(sp, pthis->buffer_, len);
            }

            if(generator->is_eof()){
              std::unique_lock<std::mutex> lock(pthis->send_data_buffer_mutex_);
              if(pthis->send_data_buffer_.empty()){
                if(state_t::closed == pthis->current_state_.change_state(
                    [](state_t & state) -> bool {
                      switch (state) {
                        case state_t::write_pending:
                          state = state_t::bof;
                          return true;
                        case state_t::closed:
                          return true;
                        default:
                          return false;
                      }
                    }, error_logic::throw_exception)){
                      return;
                }
                lock.unlock();
              }
              else {
                if(state_t::closed == pthis->current_state_.change_state(
                    [](state_t & state) -> bool {
                      switch (state) {
                        case state_t::write_pending:
                          state = state_t::write_scheduled;
                          return true;
                        case state_t::closed:
                          return true;
                        default:
                          return false;
                      }
                    }, error_logic::throw_exception)){
                  return;
                }

                lock.unlock();
                pthis->continue_send_data(sp, owner);
              }
            } else {
              if(state_t::closed == pthis->current_state_.change_state(
                  [](state_t & state) -> bool {
                    switch (state) {
                      case state_t::write_pending:
                        state = state_t::write_scheduled;
                        return true;
                      case state_t::closed:
                        return true;
                      default:
                        return false;
                    }
                  }, error_logic::throw_exception)){
                return;
              }

              pthis->continue_send_data(sp, owner);
            }
          }
      );
}

void vds::_udp_transport_queue::send_data(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> &owner,
    const std::shared_ptr<_p2p_crypto_tunnel> &session,
    const vds::const_data_buffer &data) {

  this->emplace(
    sp,
    owner,
    new data_datagram(session, data));
}

void vds::_udp_transport_queue::emplace(
    const service_provider & sp,
    const std::shared_ptr<_udp_transport> & owner,
    vds::_udp_transport_queue::datagram_generator *item) {

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  this->send_data_buffer_.push_back(std::move(std::unique_ptr<datagram_generator>(item)));
  lock.unlock();

  if(state_t::start_write == this->current_state_.change_state(
      [](state_t & state)->bool{
        switch (state){
          case state_t::bof:
            state = state_t::start_write;
            return true;
          case state_t::write_scheduled:
          case state_t::write_pending:
            return true;
          case state_t::closed:
            state = state_t::closed;
            return true;
          default:
            return false;
        }
      },
      error_logic::throw_exception)) {
        this->continue_send_data(sp, owner);
  }
}

void vds::_udp_transport_queue::stop(const vds::service_provider &sp) {
  if(state_t::close_pending == this->current_state_.change_state(
      [](state_t & state)->bool {
        switch (state) {
          case state_t::bof:
            state = state_t::closed;
            return true;
          case state_t::closed:
            return true;
          case state_t::write_scheduled:
            state = state_t::close_pending;
            return true;
          default:
            return false;
        }
      }, error_logic::throw_exception)) {
    this->current_state_.wait(
        state_t::closed,
        error_logic::throw_exception);
  }

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  while(!this->send_data_buffer_.empty()) {
    this->send_data_buffer_.pop_front();
  }

  sp.get<logger>()->trace("P2PUDP", sp, "UDPtransport_queue::stop");
}

uint16_t vds::_udp_transport_queue::data_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {

  auto seq_number = this->owner()->output_sequence_number();
  *(uint32_t *)buffer = htonl(seq_number);

  sp.get<logger>()->trace("P2PUDP", sp, "[%d] Send data to %s",
                          seq_number,
                          this->owner()->address().to_string().c_str());

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
    *(uint16_t *)(buffer + 4) = htons(this->data_.size());
    auto size = (size_t)(this->owner()->mtu() - 6);
    if(size > this->data_.size()){
      size = this->data_.size();
    }
	sp.get<logger>()->trace("P2PUDP", sp, "[%d] Send data %d of %d",
		seq_number,
		size,
		this->data_.size());

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
	sp.get<logger>()->trace("P2PUDP", sp, "[%d] Send data %d of %d offset %d",
		seq_number,
		size,
		this->data_.size(),
		this->offset_);

    memcpy(buffer + 4, this->data_.data() + this->offset_, size);
    this->offset_ += size;
    return size + 4;
  }
}

void vds::_udp_transport_queue::data_datagram::complete(const service_provider &sp, const uint8_t *buffer, size_t len) {
  this->owner()->add_datagram(const_data_buffer(buffer, len));
}

uint16_t
vds::_udp_transport_queue::acknowledgement_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send acknowledgement to %s",
                          this->owner()->address().to_string().c_str());

  auto owner_ = this->owner();

  uint32_t result_mask;
  auto seq = owner_->report_incoming_sequence(result_mask);

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Acknowledgement) << 28
      | safe_cast<uint32_t>(seq, 0x0FFFFFFF));

  *(reinterpret_cast<uint32_t *>(buffer + 4)) = htonl(result_mask);

  return 8;
}

uint16_t vds::_udp_transport_queue::handshake_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send handshake to %s",
                          this->owner()->address().to_string().c_str());

  if(16 != this->instance_id_.size()){
    throw std::runtime_error("Invalid GUID size");
  }

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Handshake) << 28
      | udp_transport::protocol_version);
  memcpy(buffer + 4, this->instance_id_.data(), this->instance_id_.size());

  return safe_cast<uint16_t>(4 + this->instance_id_.size());
}

void vds::_udp_transport_queue::handshake_datagram::complete(
    const service_provider &sp,
    const uint8_t * buffer,
    size_t len) {
  this->owner()->handshake_sent(sp);
}

uint16_t vds::_udp_transport_queue::welcome_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send welcome to %s",
                          this->owner()->address().to_string().c_str());

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Welcome) << 28
      | udp_transport::protocol_version);
  memcpy(buffer + 4, this->instance_id_.data(), this->instance_id_.size());

  return safe_cast<uint16_t>(4 + this->instance_id_.size());
}

void vds::_udp_transport_queue::welcome_datagram::complete(const service_provider &sp, const uint8_t *buffer, size_t len) {
  this->owner()->welcome_sent(sp);
}

uint16_t
vds::_udp_transport_queue::keep_alive_datagram::generate_message(const vds::service_provider &sp, uint8_t *buffer) {
  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Keep_alive) << 28
      | safe_cast<uint32_t>(
          this->owner()->output_sequence_number(), 0x0FFFFFFF));

  return 4;
}

uint16_t
vds::_udp_transport_queue::repeat_datagram::generate_message(
    const vds::service_provider &sp,
    uint8_t * buffer) {
  return this->owner()->get_sent_data(
      buffer, this->sequence_number_);
}

uint16_t
vds::_udp_transport_queue::failed_datagram::generate_message(const vds::service_provider &sp, uint8_t *buffer) {
  *buffer = (uint8_t)_udp_transport_session::control_type::Failed << 4;
  return 1;
}
