//
// Created by vadim on 31.10.17.
//

#include <netinet/in.h>
#include "udp_transport.h"
#include "private/udp_transport_p.h"
#include "binary_serialize.h
#include "udp_socket.h"

static constexpr uint8_t protocol_version = 0;
///////////////////////////////////////////////////////
void vds::_udp_transport::start(const service_provider & sp) {
  this->continue_read_outgoing_stream(sp);

  this->send(sp, this->next_message());
}

vds::const_data_buffer vds::_udp_transport::next_message() {
}

void vds::_udp_transport::send(const service_provider & sp, const const_data_buffer & data)
{

}

void vds::_udp_transport::continue_read_outgoing_stream(const service_provider & sp) {
  this->outgoing_stream_.read_async(
          sp,
          this->outgoing_stream_buffer_ + this->outgoing_stream_buffer_count_,
          sizeof(this->outgoing_stream_buffer_) / sizeof(this->outgoing_stream_buffer_[0])
          - this->outgoing_stream_buffer_count_)
      .execute([pthis = this->shared_from_this(), sp](size_t readed) {
        std::lock_guard<std::mutex> lock(pthis->outgoing_stream_state_mutex_);
        if (0 == readed) {
          pthis->outgoing_stream_state_ = outgoing_stream_state::eof;
          //EOF
        } else {
          pthis->outgoing_stream_buffer_count_ += readed;
          if(sizeof(pthis->outgoing_stream_buffer_) / sizeof(pthis->outgoing_stream_buffer_[0])
             < pthis->outgoing_stream_buffer_count_){
            pthis->continue_read_outgoing_stream(sp);
          }
        }
      });
}

void vds::_udp_transport::send_data(
    const service_provider & sp,
    const std::shared_ptr<session> & session,
    const const_data_buffer & data)
{
  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto need_to_start = this->send_data_buffer_.empty();
  this->send_data_buffer_.emplace(datagram {
      .owner_ = session,
      .data_ = data,
      .offset_ = 0
  });
  lock.unlock();

  if(need_to_start){
    this->continue_send_data(sp);
  }
}

vds::async_task<> vds::_udp_transport::session::incomming_message(
    const service_provider & sp,
    const uint8_t * data,
    uint16_t size)
{
  if(4 > size){
    return async_task<>(std::make_shared<std::runtime_error>("Small message"));
  }

  if(0x80 == (0x80 & data[0])){
    switch((control_type)(data[0] >> 4)){
      case control_type::Handshake:
        break;

    }
  } else {
    auto seq = *(uint32_t *)data;

    std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);
    if(this->future_data_.end() == this->future_data_.find(seq)) {
      this->future_data_[seq] = const_data_buffer(data + 4, size - 4);
      return this->continue_process_incoming_data(sp);
    } else {
      return async_task<>::empty();
    }
  }
}

vds::async_task<> vds::_udp_transport::session::continue_process_incoming_data(
    const service_provider & sp){

  if(this->future_data_.empty()
     || this->min_incoming_sequence_ != this->future_data_.begin()->first) {
    return async_task<>::empty();
  }

  auto data = this->future_data_.begin()->second;
  this->min_incoming_sequence_++;
  this->future_data_.erase(this->future_data_.begin());

  return this->push_data(sp, data).then([pthis = this->shared_from_this(), sp](){
    std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);
    return pthis->continue_process_incoming_data(sp);
  });
}


void vds::_udp_transport::session::on_timer(const std::shared_ptr<_udp_transport> & owner) {
  std::unique_lock<std::mutex> lock(this->incoming_sequence_mutex_);
  std::unique_lock<std::mutex> owner_lock(owner->send_data_buffer_mutex_);

  owner->send_data_buffer_.emplace(std::make_shared<acknowledgement_datagram>(
      this->shared_from_this(),
      this->min_incoming_sequence_,
      this->future_data_.empty()
      ? this->min_incoming_sequence_
      : this->future_data_.rbegin()->first));
}

vds::async_task<> vds::_udp_transport::session::push_data(
    const service_provider & sp,
    const const_data_buffer & data){
  const uint8_t * p;
  size_t s;

  if(0 == this->expected_size_){
    this->expected_size_ = ntohs(*reinterpret_cast<const uint16_t *>(data.data()));
    this->expected_buffer_.reset(this->expected_size_);
    this->expected_buffer_p_ = const_cast<uint8_t *>(this->expected_buffer_.data());
    p = data.data() + 2;
    s = data.size() - 2;
  } else {
    p = data.data();
    s = data.size();
  }

  if(this->expected_size_ < s){
    return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
  }

  memcpy(this->expected_buffer_p_, p, s);
  this->expected_buffer_p_ += s;
  this->expected_size_ -= s;

  if(0 == this->expected_size_){
    return this->socket_.write_async(
        sp,
        this->expected_buffer_.data(),
        this->expected_buffer_.size());
  } else {
    return async_task<>::empty();
  }
}

void vds::_udp_transport::continue_send_data(const service_provider & sp) {

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto & generator = this->send_data_buffer_.front();
  lock.unlock();

  auto len = generator->generate_message(this->buffer_);
  this->socket_.send(sp,
      udp_datagram::create(generator->owner()->address().server,
                           generator->owner()->address().port,
                           this->buffer_,
                           len))
      .execute(
      [pthis = this->shared_from_this(), sp, generator](
          const std::shared_ptr<std::exception> & ex){
        if(ex){
          auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
          if(datagram_error){
            session->decrease_mtu();
          } else {
            //TODO: close connection
          }
        } else {
          session->mtu(seq_number + 1);
        }

        if(message.offset_ >= message.data_.size()){
          std::unique_lock<std::mutex> lock(pthis->send_data_buffer_mutex_);
          pthis->send_data_buffer_.pop();
          auto data_eof = pthis->send_data_buffer_.empty();
          lock.unlock();

          session->message_sent();

          if(!data_eof){
            pthis->continue_send_data(sp);
          }

        } else {
          pthis->continue_send_data(sp);
        }
      }
  );
}


void vds::_udp_transport::continue_read_socket(const service_provider & sp)
{
  this->socket_.incoming()->read_async(sp, &this->incomming_buffer_, 1)
      .execute([sp, pthis = this->shared_from_this()](size_t readed){
        std::unique_lock<std::mutex> lock(pthis->sessions_mutex_);
        if(0 == readed) {
          pthis->incomming_eof_ = true;
        }
        else {
          address addr(pthis->incomming_buffer_.server(), pthis->incomming_buffer_.port());
          auto p = pthis->sessions_.find(addr);
          if(pthis->sessions_.end() == p){
            auto session = std::make_shared<session>(addr);
            pthis->sessions_[addr] = session;
            lock.unlock();

            session->incoming_data(pthis->incomming_buffer_.data());
          } else {
            auto session = p->second;
            lock.unlock();

            session->incoming_data(pthis->incomming_buffer_.data());
          }

          pthis->continue_read_socket(sp);
        }
      });
}
