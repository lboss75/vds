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

  auto len = generator->generate_message(this->buffer_);
  owner->write_async(
          udp_datagram(generator->owner()->address().server_,
                       generator->owner()->address().port_,
                       this->buffer_,
                       len))
      .execute(
          [pthis = this->shared_from_this(), sp, owner, generator, len](
              const std::shared_ptr<std::exception> & ex){
            if(ex){
              auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
              if(datagram_error){
                generator->owner()->decrease_mtu();
              } else {
                owner->close_session(generator->owner().get(), ex);
              }
            } else {
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
    const std::shared_ptr<_udp_transport_session> &session,
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

uint16_t vds::_udp_transport_queue::data_datagram::generate_message(uint8_t *buffer) {

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
    auto size = (size_t)(this->owner()->mtu() - 6);
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

uint16_t vds::_udp_transport_queue::acknowledgement_datagram::generate_message(uint8_t *buffer) {
  throw std::runtime_error("Not implemented");
  /*,
  this->min_incoming_sequence_,
  this->future_data_.empty()
  ? this->min_incoming_sequence_
  : this->future_data_.rbegin()->first
*/

}

uint16_t vds::_udp_transport_queue::handshake_datagram::generate_message(uint8_t * buffer) {
  if(16 != this->instance_id_.size()){
    throw std::runtime_error("Invalid GUID size");
  }

  *(reinterpret_cast<uint32_t *>(buffer)) = htonl(
      ((uint32_t)_udp_transport_session::control_type::Handshake) << 24
      | udp_transport::protocol_version);
  memcpy(buffer + 4, this->instance_id_.data(), this->instance_id_.size());

  return safe_cast<uint16_t>(4 + this->instance_id_.size());
}

void vds::_udp_transport_queue::handshake_datagram::complete(
    const uint8_t * buffer, size_t len) {
}
