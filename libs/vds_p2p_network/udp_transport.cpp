//
// Created by vadim on 31.10.17.
//

#include "udp_transport.h"
#include "private/udp_transport_p.h"
#include "binary_serialize.h
#include "../../kernel/vds_network/udp_socket.h"

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
          this->outgoing_stream_buffer_count_ += readed;
          if(sizeof(pthis->outgoing_stream_buffer_) / sizeof(pthis->outgoing_stream_buffer_[0])
             < pthis->outgoing_stream_buffer_count_){
            pthis->continue_read_outgoing_stream(sp);
          }
        }
      });
}

void vds::_udp_transport::send_data(const service_provider & sp, const udp_datagram & data)
{
  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  auto need_to_start = this->send_data_buffer_.empty();
  this->send_data_buffer_.push_back(data);
  lock.unlock();

  if(need_to_start){
    this->continue_send_data(sp);
  }
}

void vds::_udp_transport::continue_send_data(const service_provider & sp) {

  std::unique_lock<std::mutex> lock(this->send_data_buffer_mutex_);
  udp_datagram message = this->send_data_buffer_.front();
  lock.unlock();

  this->socket_.send(sp, message).execute(
      [pthis = this->shared_from_this(), sp](const std::shared_ptr<std::exception> & ex){
        std::unique_lock<std::mutex> lock(pthis->send_data_buffer_mutex_);
        pthis->send_data_buffer_.pop();
        auto data_eof = pthis->send_data_buffer_.empty();
        lock.unlock();

        if(ex){
          auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
          if(datagram_error){
            address addr(message.server(), message.port());

            std::unique_lock<std::mutex> lock(pthis->sessions_mutex_);
            auto p = pthis->sessions_.find(addr);
            if(pthis->sessions_.end() != p) {
              auto session = p->second;
              lock.unlock();

              session->decrease_mtu();
            }
          }
        }

        if(!data_eof){
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
