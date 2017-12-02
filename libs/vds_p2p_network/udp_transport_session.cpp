#include "stdafx.h"
#include "private/udp_transport_session_p.h"
#include "private/udp_transport_p.h"

void vds::_udp_transport_session::incomming_message(
    const service_provider &sp,
    _udp_transport &owner,
    const uint8_t *data,
    uint16_t size)
{
  if(4 > size){
    throw std::runtime_error("Small message");
  }

  if(0x80 == (0x80 & data[0])){
    switch((control_type)(data[0] >> 4)){
      case control_type::Handshake:
        break;

    }
  } else {
    auto seq = ntohl(*(uint32_t *)data);

    std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);
    if(this->future_data_.end() == this->future_data_.find(seq)) {
      this->future_data_[seq] = const_data_buffer(data + 4, size - 4);
      this->continue_process_incoming_data(sp, owner);
    }
  }
}

void vds::_udp_transport_session::continue_process_incoming_data(
    const service_provider &sp,
    _udp_transport &owner) {

  for(;;) {
    if (this->future_data_.empty()
        || this->min_incoming_sequence_ != this->future_data_.begin()->first) {
      return;
    }

    auto data = this->future_data_.begin()->second;
    this->min_incoming_sequence_++;
    this->future_data_.erase(this->future_data_.begin());

    const uint8_t *pdata;
    size_t size;
    if (0 == this->expected_size_) {
      this->expected_size_ = ntohs(*reinterpret_cast<const uint16_t *>(data.data()));
      pdata = data.data() + 2;
      size = data.size() - 2;
    } else {
      pdata = data.data();
      size = data.size();
    }

    if (size < this->expected_size_) {
      this->expected_buffer_.add(pdata, size);
      this->expected_size_ -= size;
    } else if (size != this->expected_size_) {
      throw std::runtime_error("Invalid data size");
    } else {
      if (0 == this->expected_buffer_.size()) {
        this->process_incoming_datagram(sp, owner, pdata, size);
      } else {
        this->expected_buffer_.add(pdata, size);
        this->process_incoming_datagram(sp, owner, this->expected_buffer_.data(),
                                        this->expected_buffer_.size());
      }

      this->expected_size_ = 0;
      this->expected_buffer_.clear();
    }
  }
}


void vds::_udp_transport_session::on_timer(
    const service_provider & sp,
    const std::shared_ptr<_udp_transport> & owner) {
  std::unique_lock<std::mutex> lock(this->state_mutex_);
  switch(this->current_state_){
    case send_state::bof:
    {
      owner->send_queue()->emplace(
          sp,
          owner,
          new _udp_transport_queue::handshake_datagram(
              this->shared_from_this(),
              owner->instance_id_));
      break;
    }
    case send_state ::wait_message:
    {
      owner->send_queue()->emplace(
          sp,
          owner,
          new _udp_transport_queue::acknowledgement_datagram(this->shared_from_this()));
      break;
    }
    default:
      throw std::runtime_error("Not implemented");
  }
}

void vds::_udp_transport_session::process_incoming_datagram(
    const vds::service_provider &sp,
    class _udp_transport &owner,
    const uint8_t *data,
    size_t size) {
  owner.process_incoming_datagram(sp, data, size);
}

void vds::_udp_transport_session::decrease_mtu() {

}

void vds::_udp_transport_session::send_handshake(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {
  sp.get<logger>()->trace("UDP", sp, "Send handshake to %s:%d",
                          this->address_.server_.c_str(),
                          this->address_.port_);

  owner->send_queue()->emplace(
      sp,
      owner,
      new _udp_transport_queue::handshake_datagram(
          this->shared_from_this(),
          owner->instance_id_));
}


