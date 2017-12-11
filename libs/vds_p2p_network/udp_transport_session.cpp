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
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s:%d: incomming handshake",
                                this->address_.server_.c_str(),
                                this->address_.port_);
        std::unique_lock<std::mutex> lock(this->incoming_state_mutex_);
        if(incoming_state::bof == this->incoming_state_) {
          this->incoming_state_ = incoming_state::handshake_received;

          std::unique_lock<std::mutex> out_lock(this->send_state_mutex_);
          switch(this->send_state_){
            case send_state::bof:
            case send_state::handshake_pending:
              this->send_welcome(sp, owner.shared_from_this());
              break;
          }
          out_lock.unlock();

          owner.handshake_completed(sp, this);
        }

        break;
      }
      case control_type::Welcome:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s:%d: incomming welcome",
                                this->address_.server_.c_str(),
                                this->address_.port_);
        if(20 != size){
          throw std::runtime_error("Invalid message");
        }

        std::unique_lock<std::mutex> lock(this->send_state_mutex_);
        if(send_state::handshake_pending == this->send_state_
            || send_state::bof == this->send_state_) {
          guid instance_id(data + 4, 16);
          if(0 == this->instance_id_.size()) {
            this->instance_id_ = instance_id;
            this->send_state_ = send_state::wait_message;

            owner.handshake_completed(sp, this);
          }
          else if(this->instance_id_ != instance_id){
            throw std::runtime_error("Invalid message");
          }
        }
        else {
          throw std::runtime_error("Invalid state");
        }

        break;
      }
      case control_type::Keep_alive:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s:%d: incoming keep alive",
                                this->address_.server_.c_str(),
                                this->address_.port_);
        if(4 != size){
          throw std::runtime_error("Invalid message");
        }

        uint32_t sequence_number = 0x0FFFFFFF & ntohl(*(uint32_t *)data);
        owner.send_queue()->emplace(
            sp,
            owner.shared_from_this(),
            new _udp_transport_queue::acknowledgement_datagram(
                this->shared_from_this()));
        break;
      }
      case control_type::Acknowledgement:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s:%d: incoming acknowledgement",
                                this->address_.server_.c_str(),
                                this->address_.port_);
        if(8 != size){
          throw std::runtime_error("Invalid message");
        }

        uint32_t sequence_number = 0x0FFFFFFF & ntohl(*(uint32_t *)data);
        uint32_t result_mask = ntohl(*(uint32_t *)(data + 4));

        std::unique_lock<std::mutex> lock(this->output_sequence_mutex_);
        while(!this->sent_data_.empty()
              && sequence_number > this->sent_data_.begin()->first){
          this->sent_data_.erase(this->sent_data_.begin());
        }
        for(int i = 0; i < 32; ++i){
          if(++sequence_number > this->output_sequence_number_){
            break;
          }

          if(0 != (result_mask & 1)){
            this->sent_data_.erase(sequence_number);
          }
          else {
            owner.send_queue()->emplace(
                sp,
                owner.shared_from_this(),
                new _udp_transport_queue::repeat_datagram(
                    this->shared_from_this(),
                    sequence_number));
          }
          result_mask >>= 1;
        }

        break;
      }
      default:
        throw std::runtime_error("Not implemented");

    }
  } else {
    auto seq = ntohl(*(uint32_t *)data);
    sp.get<logger>()->trace("P2PUDP", sp, "%s:%d: incomming data %d",
                            this->address_.server_.c_str(),
                            this->address_.port_,
                            seq);

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
      std::unique_lock<std::mutex> lock(this->incoming_datagram_mutex_);
      if (0 == this->expected_buffer_.size()) {
        this->incoming_datagrams_.push(const_data_buffer(pdata, size));
      } else {
        this->expected_buffer_.add(pdata, size);
        this->incoming_datagrams_.push(const_data_buffer(this->expected_buffer_.data(),
                                        this->expected_buffer_.size()));
      }
      this->incoming_datagram_mutex_.unlock();

      this->expected_size_ = 0;
      this->expected_buffer_.clear();

      this->try_read_data();
    }
  }
}


void vds::_udp_transport_session::on_timer(
    const service_provider & sp,
    const std::shared_ptr<_udp_transport> & owner) {
  std::unique_lock<std::mutex> lock(this->send_state_mutex_);
  switch(this->send_state_){
    case send_state::bof:
    {
      this->send_state_ = send_state::handshake_pending;
      this->send_handshake(sp, owner);
      break;
    }
    case send_state::handshake_pending:
    {
      break;
    }
    case send_state::welcome_sent:
    {
      this->send_state_ = send_state::handshake_pending;
      this->send_handshake(sp, owner);
      break;
    }
    case send_state::welcome_pending:
    {
      break;
    }
    case send_state ::wait_message:
    {
      owner->send_queue()->emplace(
          sp,
          owner,
          new _udp_transport_queue::keep_alive_datagram(
              this->shared_from_this()));
      break;
    }
    default:
      throw std::runtime_error("Not implemented");
  }
}

void vds::_udp_transport_session::decrease_mtu() {
  throw std::runtime_error("Not implemented");
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

void vds::_udp_transport_session::handshake_sent() {
  std::unique_lock<std::mutex> lock(this->send_state_mutex_);
  this->send_state_ = send_state::bof;
}

void vds::_udp_transport_session::send_welcome(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {
  sp.get<logger>()->trace("UDP", sp, "Send welcome to %s:%d",
                          this->address_.server_.c_str(),
                          this->address_.port_);

  owner->send_queue()->emplace(
      sp,
      owner,
      new _udp_transport_queue::welcome_datagram(
          this->shared_from_this(),
          owner->instance_id_));
}

void vds::_udp_transport_session::welcome_sent() {
  std::unique_lock<std::mutex> lock(this->send_state_mutex_);
  this->send_state_ = send_state::welcome_sent;
}

vds::_udp_transport_session::~_udp_transport_session() {
  std::cout << "_udp_transport_session::~_udp_transport_session\n";
}

void vds::_udp_transport_session::send(
    const service_provider &sp,
    const const_data_buffer &message) {
  auto owner = this->owner_.lock();
  owner->send_queue()->emplace(
      sp,
      owner,
      new _udp_transport_queue::data_datagram(
          this->shared_from_this(),
          owner->instance_id_));
}

vds::async_task<const vds::const_data_buffer &> vds::_udp_transport_session::read_async(
    const vds::service_provider &sp) {

  return [pthis = this->shared_from_this()](const async_result<const vds::const_data_buffer &> & result){
    auto this_ = static_cast<_udp_transport_session *>(pthis.get());
    std::unique_lock<std::mutex> lock(this_->read_result_mutex_);
    this_->read_result_ = result;
    lock.unlock();

    this_->try_read_data();
  };
}

void vds::_udp_transport_session::try_read_data() {
  std::unique_lock<std::mutex> data_lock(this->incoming_datagram_mutex_);
  if(this->incoming_datagrams_.empty()){
    return;
  }

  std::unique_lock<std::mutex> lock(this->read_result_mutex_);
  if(!this->read_result_){
    return;
  }

  auto result = std::move(this->read_result_);
  lock.unlock();

  const_data_buffer message(this->incoming_datagrams_.front());
  this->incoming_datagrams_.pop();

  data_lock.unlock();

  result.done(message);
}

uint16_t vds::_udp_transport_session::get_sent_data(
    uint8_t *buffer,
    uint32_t sequence_number) {
  std::unique_lock<std::mutex> lock(this->output_sequence_mutex_);
  auto p = this->sent_data_.find(sequence_number);
  if(this->sent_data_.end() == p){
    return 0;
  }

  memcpy(buffer, p->second.data(), p->second.size());
  return p->second.size();
}


