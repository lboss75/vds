//
// Created by vadim on 31.10.17.
//
#include <network_service.h>
#include "stdafx.h"
#include "udp_transport.h"
#include "private/udp_transport_p.h"
#include "binary_serialize.h"
#include "udp_socket.h"
#include "udp_datagram_size_exception.h"

static constexpr uint32_t protocol_version = 0;
///////////////////////////////////////////////////////
vds::_udp_transport::_udp_transport(
    udp_socket && socket)
: socket_(std::move(socket)), instance_id_(guid::new_guid())
{
}

void vds::_udp_transport::start(const service_provider & sp) {
  this->socket_.read_async().execute(
      [pthis = this->shared_from_this(), sp](
          const std::shared_ptr<std::exception> & ex,
          const udp_datagram & message){
        if(!ex) {
          pthis->process_incommig_message(sp, message);
        }
      }
  );
}

void vds::_udp_transport::send_broadcast(int port) {

  this->socket_.send_broadcast(port, this->create_handshake_message());

}

vds::const_data_buffer vds::_udp_transport::create_handshake_message() {
  if(16 != this->instance_id_.size()){
    throw std::runtime_error("Invalid GUID size");
  }

  uint8_t data[20];
  *(reinterpret_cast<uint32_t *>(data)) = htonl(((uint32_t)session::control_type::Handshake) << 24 | protocol_version);
  memcpy(data + 4, this->instance_id_.data(), this->instance_id_.size());

  return const_data_buffer(data, sizeof(data));
}

void vds::_udp_transport::process_incommig_message(
    const service_provider &sp,
    const udp_datagram &message) {
  if(4 > message.data_size()){
    this->socket_.read_async().execute(
        [pthis = this->shared_from_this(), sp](
            const std::shared_ptr<std::exception> & ex,
            const udp_datagram & message){
          if(!ex) {
            pthis->process_incommig_message(sp, message);
          }
        }
    );

    return;
  }

  address_t server_address(message.server(), message.port());

  std::unique_lock<std::mutex> lock(this->sessions_mutex_);

  if(0x80 == (0x80 & static_cast<const uint8_t *>(message.data())[0])
    && session::control_type::Handshake == (session::control_type)(static_cast<const uint8_t *>(message.data())[0] >> 4)){

    auto version = 0x0FFFFFFF & ntohl(*reinterpret_cast<const uint32_t *>(message.data()));

    if(version == protocol_version && 20 == message.data_size()){
      guid instance_id(message.data(), 16);
      if(instance_id != this->instance_id_){
        auto new_session = std::make_shared<session>(instance_id, server_address);
        this->sessions_[server_address] = new_session;
      }
    }

  } else {
    auto p = this->sessions_.find(server_address);
    if(this->sessions_.end() != p){
      auto session_handle = p->second;
      lock.unlock();

      try {
        session_handle->incomming_message(sp, *this, message.data(), message.data_size());
      }
      catch (...){
        std::unique_lock<std::mutex> lock(this->sessions_mutex_);
        auto p = this->sessions_.find(server_address);
        if(this->sessions_.end() != p && session_handle.get() == p->second.get()){
          this->sessions_.erase(p);
        }
      }
    }
  }
}

void vds::_udp_transport::send_data(
    const service_provider & sp,
    const std::shared_ptr<session> & session,
    const const_data_buffer & data)
{
  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto need_to_start = this->send_data_buffer_.empty();
  this->send_data_buffer_.emplace(new data_datagram(session, data));
  lock.unlock();

  if(need_to_start){
    this->continue_send_data(sp);
  }
}

void vds::_udp_transport::process_incoming_datagram(
    const vds::service_provider &sp,
    const uint8_t *data,
    size_t size) {

}

/////////////////////////////////////////////////////////////////////////
void vds::_udp_transport::session::incomming_message(
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

void vds::_udp_transport::session::continue_process_incoming_data(
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


void vds::_udp_transport::session::on_timer(const std::shared_ptr<_udp_transport> & owner) {
  std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);
  std::unique_lock<std::mutex> owner_lock(owner->send_data_buffer_mutex_);

  owner->send_data_buffer_.emplace(new acknowledgement_datagram(
      this->shared_from_this(),
      this->min_incoming_sequence_,
      this->future_data_.empty()
      ? this->min_incoming_sequence_
      : this->future_data_.rbegin()->first));
}

void vds::_udp_transport::session::process_incoming_datagram(
    const vds::service_provider &sp,
    _udp_transport &owner,
    const uint8_t *data,
    size_t size) {
  owner.process_incoming_datagram(sp, data, size);
}

void vds::_udp_transport::continue_send_data(const service_provider & sp) {

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto generator = this->send_data_buffer_.front().get();
  lock.unlock();

  auto len = generator->generate_message(this->buffer_);
  this->socket_.write_async(
      udp_datagram(generator->owner()->address().server_,
         generator->owner()->address().port_,
         this->buffer_,
         len))
      .execute(
      [pthis = this->shared_from_this(), sp, generator, len](
          const std::shared_ptr<std::exception> & ex){
        if(ex){
          auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
          if(datagram_error){
            generator->owner()->decrease_mtu();
          } else {
            //TODO: close connection
          }
        } else {
          generator->complete(const_data_buffer(pthis->buffer_, len));
        }

        if(generator->is_eof()){
          std::unique_lock<std::mutex> lock(pthis->send_data_buffer_mutex_);
          pthis->send_data_buffer_.pop();
          auto data_eof = pthis->send_data_buffer_.empty();
          lock.unlock();

          if(!data_eof){
            pthis->continue_send_data(sp);
          }

        } else {
          pthis->continue_send_data(sp);
        }
      }
  );
}
