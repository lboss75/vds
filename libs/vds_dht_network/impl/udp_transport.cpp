/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "private/udp_transport.h"
#include "messages/dht_route_messages.h"
#include "private/dht_session.h"
#include "logger.h"
#include "dht_network_client.h"
#include "dht_network_client_p.h"

vds::dht::network::udp_transport::udp_transport(){
}

vds::dht::network::udp_transport::~udp_transport() {
}

void vds::dht::network::udp_transport::start(
  const service_provider * sp,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & node_key,
  uint16_t port) {
  this->send_thread_ = std::make_shared<thread_apartment>(sp);
  this->sp_ = sp;
  this->this_node_id_ = node_cert->fingerprint(hash::sha256());
  this->node_cert_ = node_cert;
  this->node_key_ = node_key;

  try {
    auto [reader, writer] = this->server_.start(sp, network_address::any_ip6(port));
    this->reader_ = reader;
    this->writer_ = writer;
  }
  catch (...) {
    auto[reader, writer] = this->server_.start(sp, network_address::any_ip4(port));
    this->reader_ = reader;
    this->writer_ = writer;
  }

      this->continue_read().detach();
}

void vds::dht::network::udp_transport::stop() {
  this->server_.stop();
}

vds::async_task<void>
vds::dht::network::udp_transport::write_async( const udp_datagram& datagram) {
  auto result = std::make_shared<vds::async_result<void>>();
  this->send_thread_->schedule([result, this, datagram]() {
    try{
      this->writer_->write_async(datagram).get();
    }
    catch (...){
      result->set_exception(std::current_exception());
      return;
    }
    result->set_value();
  });

  return result->get_future();

  //  std::unique_lock<std::debug_mutex> lock(this->write_mutex_);
  //  while(this->write_in_progress_) {
  //    this->write_cond_.wait(*reinterpret_cast<std::unique_lock<std::mutex> *>(&lock));
  //  }
  //  this->write_in_progress_ = true;
  //#ifdef _DEBUG
  //#ifndef _WIN32
  //  this->owner_id_ = syscall(SYS_gettid);
  //#else
  //  this->owner_id_ = GetCurrentThreadId();
  //#endif
  //#endif//_DEBUG
  //co_await this->writer_->write_async(sp, datagram);
}

vds::async_task<void> vds::dht::network::udp_transport::try_handshake(
                                                                  const std::string& address) {

  resizable_data_buffer out_message;
  out_message.add((uint8_t)protocol_message_type_t::HandshakeBroadcast);
  out_message.add(MAGIC_LABEL >> 24);
  out_message.add(MAGIC_LABEL >> 16);
  out_message.add(MAGIC_LABEL >> 8);
  out_message.add(MAGIC_LABEL);
  out_message.add(PROTOCOL_VERSION);

  binary_serializer bs;
  bs << this->node_cert_->der();

  out_message += bs.move_data();

  co_await this->write_async(udp_datagram(
    network_address::parse(this->server_.address().family(), address),
    out_message.move_data(),
    false));
}

vds::async_task<void> vds::dht::network::udp_transport::on_timer() {
  std::list<std::shared_ptr<dht_session>> sessions;

  this->sessions_mutex_.lock_shared();
  for(auto & p : this->sessions_) {
    if (p.second.session_) {
      sessions.push_back(p.second.session_);
    }
  }
  this->sessions_mutex_.unlock_shared();

  for(auto & s : sessions) {
    co_await s->on_timer(this->shared_from_this());
  }
}

void vds::dht::network::udp_transport::get_session_statistics(session_statistic& session_statistic) {
  session_statistic.output_size_ = this->send_thread_->size();

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for (const auto& p : this->sessions_) {
    const auto & session = p.second;
    if (session.session_) {
      session_statistic.items_.push_back(session.session_->get_statistic());
    }
  }
}


vds::async_task<void> vds::dht::network::udp_transport::continue_read(
  ) {
  for (;;) {
    udp_datagram datagram;
    try {
      datagram = co_await this->reader_->read_async();
    }
    catch(const std::system_error & ex) {
      continue;
    }

    if (this->sp_->get_shutdown_event().is_shuting_down()) {
      co_return;
    }

    this->sessions_mutex_.lock();
    auto & session_info = this->sessions_[datagram.address()];
    this->sessions_mutex_.unlock();

    session_info.session_mutex_.lock();
    if (session_info.blocked_) {
      if ((std::chrono::steady_clock::now() - session_info.update_time_) > std::chrono::minutes(1)) {
        session_info.blocked_ = false;
      }
      else {
        session_info.session_mutex_.unlock();
        if (*datagram.data() != (uint8_t)protocol_message_type_t::Failed) {
          uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
          try {
            co_await this->write_async(udp_datagram(datagram.address(),
              const_data_buffer(out_message, sizeof(out_message))));
          }
          catch (...) {
          }
        }
        continue;
      }
    }

    switch ((protocol_message_type_t)datagram.data()[0]) {
    case protocol_message_type_t::HandshakeBroadcast:
    case protocol_message_type_t::Handshake: {

      if (session_info.session_) {
        session_info.session_mutex_.unlock();
        continue;
      }

      if (
        (uint8_t)(MAGIC_LABEL >> 24) == datagram.data()[1]
        && (uint8_t)(MAGIC_LABEL >> 16) == datagram.data()[2]
        && (uint8_t)(MAGIC_LABEL >> 8) == datagram.data()[3]
        && (uint8_t)(MAGIC_LABEL) == datagram.data()[4]
        && PROTOCOL_VERSION == datagram.data()[5]) {
        binary_deserializer bd(datagram.data() + 6, datagram.data_size() - 6);
        const_data_buffer partner_node_cert_der;
        bd >> partner_node_cert_der;
        auto partner_node_cert = certificate::parse_der(partner_node_cert_der);
        auto partner_node_id = partner_node_cert.fingerprint(hash::sha256());
        if (partner_node_id == this->this_node_id_) {
          session_info.session_mutex_.unlock();
          break;
        }

        if (!session_info.session_key_ || (std::chrono::steady_clock::now() - session_info.update_time_) > std::chrono::minutes(10)) {
          session_info.update_time_ = std::chrono::steady_clock::now();
          session_info.session_key_.resize(32);
          crypto_service::rand_bytes(session_info.session_key_.data(), session_info.session_key_.size());
        }

        session_info.session_ = std::make_shared<dht_session>(
          this->sp_,
          datagram.address(),
          this->this_node_id_,
          partner_node_id,
          session_info.session_key_);

        this->sp_->get<logger>()->debug(ThisModule, "Add session %s", datagram.address().to_string().c_str());
        (*this->sp_->get<client>())->add_session(session_info.session_, 0);

        resizable_data_buffer out_message;
        out_message.add(static_cast<uint8_t>(protocol_message_type_t::Welcome));
        out_message.add(MAGIC_LABEL >> 24);
        out_message.add(MAGIC_LABEL >> 16);
        out_message.add(MAGIC_LABEL >> 8);
        out_message.add(MAGIC_LABEL);

        binary_serializer bs;
        bs
          << this->node_cert_->der()
          << partner_node_cert.public_key().encrypt(session_info.session_key_);

        session_info.session_mutex_.unlock();

        out_message += bs.move_data();
        co_await this->write_async(udp_datagram(datagram.address(), out_message.move_data(), false));
        co_await (*this->sp_->get<client>())->send_neighbors(
          message_create<messages::dht_find_node_response>(
            std::list<messages::dht_find_node_response::target_node>({
          messages::dht_find_node_response::target_node(partner_node_id, datagram.address().to_string(), 0) })));

        co_await this->sp_->get<imessage_map>()->on_new_session(
          partner_node_id);
      }
      else {
        session_info.session_mutex_.unlock();
        throw std::runtime_error("Invalid protocol");
      }
      break;
    }
    case protocol_message_type_t::Welcome: {
      if (datagram.data_size() > 5
        && (uint8_t)(MAGIC_LABEL >> 24) == datagram.data()[1]
        && (uint8_t)(MAGIC_LABEL >> 16) == datagram.data()[2]
        && (uint8_t)(MAGIC_LABEL >> 8) == datagram.data()[3]
        && (uint8_t)(MAGIC_LABEL) == datagram.data()[4]) {

        const_data_buffer cert_buffer;
        const_data_buffer key_buffer;
        binary_deserializer bd(datagram.data() + 5, datagram.data_size() - 5);
        bd
          >> cert_buffer
          >> key_buffer;

        auto cert = certificate::parse_der(cert_buffer);
        auto key = this->node_key_->decrypt(key_buffer);

        auto partner_id = cert.fingerprint(hash::sha256());

        //TODO: validate cert
        auto session = std::make_shared<dht_session>(
          this->sp_,
          datagram.address(),
          this->this_node_id_,
          partner_id,
          key);

        session_info.session_ = session;
        session_info.session_mutex_.unlock();

        this->sp_->get<logger>()->debug(ThisModule, "Add session %s", datagram.address().to_string().c_str());
        (*this->sp_->get<client>())->add_session(session, 0);

        co_await this->sp_->get<imessage_map>()->on_new_session(
          partner_id);
      }
      else {
        session_info.session_mutex_.unlock();
        throw std::runtime_error("Invalid protocol");
      }

      break;
    }
    case protocol_message_type_t::Failed: {
      session_info.blocked_ = true;
      (*this->sp_->get<client>())->remove_session(session_info.session_);
      session_info.session_.reset();
      session_info.update_time_ = std::chrono::steady_clock::now();
      session_info.session_mutex_.unlock();
      break;
    }
    default: {
      if (session_info.session_) {
        try {
          auto session = session_info.session_;
          session_info.session_mutex_.unlock();

          bool failed = false;
          try {
            co_await session->process_datagram(
              this->shared_from_this(),
              const_data_buffer(datagram.data(), datagram.data_size()));
          }
          catch (const std::exception & ex) {
            logger::get(this->sp_)->debug(ThisModule, "%s at process message from %s",
              ex.what(),
              datagram.address().to_string().c_str());
            failed = true;
          }

          if(failed) {
            session_info.session_mutex_.lock();
            session_info.blocked_ = true;
            (*this->sp_->get<client>())->remove_session(session_info.session_);
            session_info.session_.reset();
            session_info.update_time_ = std::chrono::steady_clock::now();
            session_info.session_mutex_.unlock();

            uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
            co_await this->write_async(udp_datagram(datagram.address(),
              const_data_buffer(out_message, sizeof(out_message))));
          }
        }
        catch (...) {
          session_info.session_mutex_.lock();
          session_info.blocked_ = true;
          (*this->sp_->get<client>())->remove_session(session_info.session_);
          session_info.session_.reset();
          session_info.update_time_ = std::chrono::steady_clock::now();
          session_info.session_mutex_.unlock();
        }
      }
      else {
        session_info.blocked_ = true;
        (*this->sp_->get<client>())->remove_session(session_info.session_);
        session_info.session_.reset();
        session_info.update_time_ = std::chrono::steady_clock::now();
        session_info.session_mutex_.unlock();

        uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
        try {
          co_await this->write_async(udp_datagram(datagram.address(),
            const_data_buffer(out_message, sizeof(out_message))));
        }
        catch (...) {
        }
      }
      break;
    }
    }
  }
}

