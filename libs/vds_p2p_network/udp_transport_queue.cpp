#include <udp_datagram_size_exception.h>
#include "stdafx.h"
#include "private/udp_transport_queue_p.h"
#include "private/udp_transport_p.h"

vds::_udp_transport_queue::_udp_transport_queue() {

}

void vds::_udp_transport_queue::continue_send_data(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto generator = this->send_data_buffer_.front().get();
  lock.unlock();

  auto len = generator->generate_message(sp, this->buffer_);
  owner->write_async(
          udp_datagram(
              static_cast<_udp_transport_session *>(generator->owner().get())->address().server_,
              static_cast<_udp_transport_session *>(generator->owner().get())->address().port_,
              this->buffer_,
              len,
              false))
      .execute(
          [pthis = this->shared_from_this(), sp, owner, generator, len](
              const std::shared_ptr<std::exception> & ex){
            auto owner_ = static_cast<_udp_transport_session *>(generator->owner().get());
            if (ex) {
              auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
              if (datagram_error) {
                owner_->decrease_mtu();
              } else {
                owner_->close(sp, ex);
              }
            } else {
              owner_->register_outgoing_traffic(len);
              generator->complete(pthis->buffer_, len);
            }

            if(generator->is_eof()){
              std::unique_lock<std::mutex> lock(pthis->send_data_buffer_mutex_);
              pthis->send_data_buffer_.pop();
              auto data_eof = pthis->send_data_buffer_.empty();
              lock.unlock();

              if(!data_eof){
                pthis->continue_send_data(sp, owner);
              }

            } else {
              pthis->continue_send_data(sp, owner);
            }
          }
      );
}

void vds::_udp_transport_queue::send_data(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> &owner,
    const std::shared_ptr<udp_transport::_session> &session,
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
  auto need_to_start = this->send_data_buffer_.empty();
  this->send_data_buffer_.emplace(item);
  lock.unlock();

  if(need_to_start){
    this->continue_send_data(sp, owner);
  }

}

uint16_t vds::_udp_transport_queue::data_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {

  auto seq_number = static_cast<_udp_transport_session *>(this->owner().get())->output_sequence_number();
  *(uint32_t *)buffer = htonl(seq_number);

  sp.get<logger>()->trace("P2PUDP", sp, "[%d] Send data to %s:%d",
                          seq_number,
                          static_cast<_udp_transport_session *>(this->owner().get())->address().server_.c_str(),
                          static_cast<_udp_transport_session *>(this->owner().get())->address().port_);

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
    auto size = (size_t)(static_cast<_udp_transport_session *>(this->owner().get())->mtu() - 6);
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
    auto size = static_cast<_udp_transport_session *>(this->owner().get())->mtu() - 4;
    if(size > this->data_.size() - this->offset_){
      size = this->data_.size() - this->offset_;
    }

    memcpy(buffer + 4, this->data_.data() + this->offset_, size);
    this->offset_ += size;
    return size + 4;
  }
}

uint16_t
vds::_udp_transport_queue::acknowledgement_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send acknowledgement to %s:%d",
                          static_cast<_udp_transport_session *>(this->owner().get())->address().server_.c_str(),
                          static_cast<_udp_transport_session *>(this->owner().get())->address().port_);

  auto owner_ = static_cast<_udp_transport_session *>(this->owner().get());

  uint32_t result_mask;
  auto seq = owner_->report_incoming_sequence(result_mask);

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Acknowledgement) << 28
      | safe_cast<uint32_t>(seq, 0x0FFFFFFF));

  *(reinterpret_cast<uint32_t *>(buffer + 4)) = htonl(result_mask);

  return 8;
}

uint16_t vds::_udp_transport_queue::handshake_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send handshake to %s:%d",
                          static_cast<_udp_transport_session *>(this->owner().get())->address().server_.c_str(),
                          static_cast<_udp_transport_session *>(this->owner().get())->address().port_);

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
    const uint8_t * buffer, size_t len) {
  static_cast<_udp_transport_session *>(this->owner().get())->handshake_sent();
}

uint16_t vds::_udp_transport_queue::welcome_datagram::generate_message(const service_provider &sp, uint8_t *buffer) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send welcome to %s:%d",
                          static_cast<_udp_transport_session *>(this->owner().get())->address().server_.c_str(),
                          static_cast<_udp_transport_session *>(this->owner().get())->address().port_);

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Welcome) << 28
      | udp_transport::protocol_version);
  memcpy(buffer + 4, this->instance_id_.data(), this->instance_id_.size());

  return safe_cast<uint16_t>(4 + this->instance_id_.size());
}

void vds::_udp_transport_queue::welcome_datagram::complete(const uint8_t *buffer, size_t len) {
  static_cast<_udp_transport_session *>(this->owner().get())->welcome_sent();
}

uint16_t
vds::_udp_transport_queue::keep_alive_datagram::generate_message(const vds::service_provider &sp, uint8_t *buffer) {
  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Keep_alive) << 28
      | safe_cast<uint32_t>(
          static_cast<_udp_transport_session *>(this->owner().get())->output_sequence_number(), 0x0FFFFFFF));

  return 4;
}

uint16_t
vds::_udp_transport_queue::repeat_datagram::generate_message(
    const vds::service_provider &sp,
    uint8_t * buffer) {
  return static_cast<_udp_transport_session *>(this->owner().get())->get_sent_data(
      buffer, this->sequence_number_);
}

uint16_t
vds::_udp_transport_queue::failed_datagram::generate_message(const vds::service_provider &sp, uint8_t *buffer) {
  *buffer = (uint8_t)_udp_transport_session::control_type::Failed << 4;
  return 1;
}
