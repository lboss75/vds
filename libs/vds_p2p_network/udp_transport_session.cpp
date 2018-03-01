#include "stdafx.h"
#include "private/udp_transport_session_p.h"
#include "private/udp_transport_p.h"

void vds::_udp_transport_session::incomming_message(
    const service_provider &sp,
    _udp_transport &owner,
    const uint8_t *data,
    uint16_t size)
{
  this->received_data_bytes_ += size;

  if(0x80 == (0x80 & data[0])){
    switch((control_type)(data[0] >> 4)){
      case control_type::Handshake:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s: incomming handshake",
                                this->address_.to_string().c_str());
        std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
        if(state_t::bof == this->current_state_) {
          this->current_state_ = state_t::welcome_pending;
          this->send_welcome(sp, owner.shared_from_this());
          owner.handshake_completed(sp, this->shared_from_this());
        }

        break;
      }
      case control_type::Welcome:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s: incomming welcome",
                                this->address_.to_string().c_str());
        if(20 != size){
          throw std::runtime_error("Invalid message");
        }

        std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
        switch (this->current_state_) {
          case state_t::handshake_sent:
          case state_t::handshake_pending:
          case state_t::welcome_pending:
          case state_t::welcome_sent: {
            guid instance_id(data + 4, 16);
            if (0 == this->instance_id_.size()) {
              this->instance_id_ = instance_id;
              this->current_state_ = state_t::wait_message;

              owner.handshake_completed(sp, this->shared_from_this());
            } else if (this->instance_id_ != instance_id) {
              throw std::runtime_error("Invalid message");
            }
            break;
          }

          case state_t::wait_message:
            break;//Ignore

          default:
            throw std::runtime_error("Invalid state");
        }

        break;
      }
      case control_type::Keep_alive:
      {
        sp.get<logger>()->trace("P2PUDP", sp, "%s: incoming keep alive",
                                this->address_.to_string().c_str());
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
        sp.get<logger>()->trace("P2PUDP", sp, "%s: incoming acknowledgement",
                                this->address_.to_string().c_str());
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
            lock.unlock();

            owner.send_queue()->emplace(
                sp,
                owner.shared_from_this(),
                new _udp_transport_queue::repeat_datagram(
                    this->shared_from_this(),
                    sequence_number));

            lock.lock();
          }
          result_mask >>= 1;
        }

        break;
      }
      case control_type::Failed:{
        this->current_state_ = state_t::fail;
        break;
      }
      default:
        throw std::runtime_error("Not implemented");

    }
  } else {
    if(4 > size){
      throw std::runtime_error("Small message");
    }

    std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
    switch (this->current_state_) {
      case state_t::welcome_pending:
      case state_t::welcome_sent:
        this->current_state_ = state_t::wait_message;
        break;

      case state_t::wait_message:
        break;

      default:
        throw std::runtime_error("Invalid state");
    }
    lock.unlock();

    auto seq = ntohl(*(uint32_t *)data);
	auto log = sp.get<logger>();
	log->trace("P2PUDP", sp, "%s: incoming data seq %d",
                            this->address_.to_string().c_str(),
                            seq);

    std::unique_lock<std::mutex> seq_lock(this->incoming_sequence_mutex_);
    if(this->future_data_.end() == this->future_data_.find(seq)) {
      this->future_data_[seq] = const_data_buffer(data + 4, size - 4);
      this->continue_process_incoming_data(sp, owner);
    }
	else {
		log->warning("P2PUDP", sp, "%s: dublicate data seq %d",
			this->address_.to_string().c_str(),
			seq);
	}
  }
}

void vds::_udp_transport_session::continue_process_incoming_data(
    const service_provider &sp,
    _udp_transport &owner) {
	auto log = sp.get<logger>();
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
	  log->trace("P2PUDP", sp, "new block [%d] %d of expected %d", this->min_incoming_sequence_, size, this->expected_size_);
	} else {
      pdata = data.data();
      size = data.size();
	  log->trace("P2PUDP", sp, "continue [%d] %d of expected %d", this->min_incoming_sequence_, size, this->expected_size_);
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
		log->trace("P2PUDP", sp, "complete %d", size);
	  } else {
        this->expected_buffer_.add(pdata, size);
        this->incoming_datagrams_.push(const_data_buffer(this->expected_buffer_.data(),
                                        this->expected_buffer_.size()));
		log->trace("P2PUDP", sp, "complete %d", this->expected_buffer_.size());
	  }
      lock.unlock();

      this->expected_size_ = 0;
      this->expected_buffer_.clear();

	  this->try_read_data();
    }
  }
}

void vds::_udp_transport_session::on_timer(
    const service_provider & sp,
    const std::shared_ptr<_udp_transport> & owner) {
  std::shared_lock<std::shared_mutex> lock(this->current_state_mutex_);
  switch(this->current_state_){
  case state_t::bof:
    case state_t::handshake_pending:
    case state_t::welcome_sent:
    case state_t::welcome_pending:
	case state_t::fail: {
		sp.get<logger>()->trace("P2PUDP", sp, "Skip timer %d", this->current_state_);
		break;
	}

	case state_t::handshake_sent: {
		if (5 < this->timer_count_++) {
			this->timer_count_ = 0;
			this->send_handshake_(sp, owner);
		}
		break;
	}
	case state_t::wait_message: {
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

void vds::_udp_transport_session::send_handshake_(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {
  if(state_t::bof != this->current_state_
	  && state_t::handshake_sent != this->current_state_) {
    throw std::runtime_error("Invalid state");
  }
  this->current_state_ = state_t::handshake_pending;

  sp.get<logger>()->trace("P2PUDP", sp, "Send handshake to %s",
                          this->address_.to_string().c_str());

  owner->send_queue()->emplace(
      sp,
      owner,
      new _udp_transport_queue::handshake_datagram(
          this->shared_from_this(),
          owner->instance_id_));
}

void vds::_udp_transport_session::handshake_sent() {
  std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
  if(state_t::handshake_pending == this->current_state_) {
    this->current_state_ = state_t::handshake_sent;
  }
}

void vds::_udp_transport_session::send_welcome(
    const vds::service_provider &sp,
    const std::shared_ptr<_udp_transport> & owner) {
  sp.get<logger>()->trace("P2PUDP", sp, "Send welcome to %s",
                          this->address_.to_string().c_str());

  owner->send_queue()->emplace(
      sp,
      owner,
      new _udp_transport_queue::welcome_datagram(
          this->shared_from_this(),
          owner->instance_id_));
}

void vds::_udp_transport_session::welcome_sent() {
  std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
  if(state_t::welcome_pending == this->current_state_) {
    this->current_state_ = state_t::welcome_sent;
  }
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
          message));
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

void vds::_udp_transport_session::register_outgoing_traffic(uint32_t bytes) {
  this->sent_data_bytes_ += bytes;
}

void vds::_udp_transport_session::close(const vds::service_provider &sp, const std::shared_ptr<std::exception> &ex) {

  this->current_state_ = state_t::fail;
  this->error_ = ex;

  auto owner = this->owner_.lock();
  if(owner){
    owner->send_queue()->emplace(
        sp,
        owner,
        new _udp_transport_queue::failed_datagram(
            this->shared_from_this()));
  }
}

void vds::_udp_transport_session::set_instance_id(const vds::guid &instance_id) {
  if(this->instance_id_){
    throw std::runtime_error("Logic error");
  }
  this->instance_id_ = instance_id;

}

bool vds::_udp_transport_session::is_failed() const {
  std::shared_lock<std::shared_mutex> lock(this->current_state_mutex_);
  return state_t::fail == this->current_state_;
}

vds::async_task<> vds::_udp_transport_session::prepare_to_stop(const vds::service_provider &sp) {
  std::unique_lock<std::shared_mutex> lock(this->current_state_mutex_);
  this->current_state_ = state_t::closed;
  this->read_result_.clear();

  return async_task<>::empty();
}
