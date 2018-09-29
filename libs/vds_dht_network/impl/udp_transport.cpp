/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "private/udp_transport.h"
#include "messages/dht_message_type.h"
#include "private/dht_session.h"
#include "logger.h"
#include "dht_network_client.h"
#include "dht_network_client_p.h"
#include "messages/dht_find_node_response.h"

vds::dht::network::udp_transport::udp_transport() {
}

void vds::dht::network::udp_transport::start(
  const service_provider& sp,
  const std::shared_ptr<certificate> & node_cert,
  const std::shared_ptr<asymmetric_private_key> & node_key,
  uint16_t port) {

  this->this_node_id_ = node_cert.fingerprint(hash::sha256());
  this->node_cert_ = node_cert;
  this->node_key_ = node_key;

  try {
    this->server_.start(sp, network_address::any_ip6(port));
  }
  catch (...) {
    this->server_.start(sp, network_address::any_ip4(port));
  }

  this->continue_read(sp.create_scope("vds::dht::network::udp_transport::continue_read"));
}

void vds::dht::network::udp_transport::stop(const service_provider& sp) {
  this->server_.stop(sp);
}

std::future<void>
vds::dht::network::udp_transport::write_async(const service_provider& sp, const udp_datagram& datagram) {
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
  co_await this->server_.socket().write_async(datagram);
}

std::future<void> vds::dht::network::udp_transport::try_handshake(const service_provider& sp,
                                                                  const std::string& address) {

  this->block_list_mutex_.lock();
  auto p = this->block_list_.find(address);
  if (this->block_list_.end() != p && p->second < std::chrono::steady_clock::now()) {
    this->block_list_mutex_.unlock();
    co_return;
  }
  this->block_list_[address] = std::chrono::steady_clock::now() + std::chrono::minutes(1);
  this->block_list_mutex_.unlock();

  resizable_data_buffer out_message;
  out_message.add((uint8_t)protocol_message_type_t::HandshakeBroadcast);
  out_message.add(MAGIC_LABEL >> 24);
  out_message.add(MAGIC_LABEL >> 16);
  out_message.add(MAGIC_LABEL >> 8);
  out_message.add(MAGIC_LABEL);
  out_message.add(PROTOCOL_VERSION);

  binary_serializer bs;
  bs << this->node_cert_.der();

  out_message += bs.move_data();

  co_await this->write_async(sp, udp_datagram(
    network_address::parse(this->server_.address().family(), address),
    out_message.move_data(),
    false));
}

void vds::dht::network::udp_transport::get_session_statistics(session_statistic& session_statistic) {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for (const auto& p : this->sessions_) {
    const auto session = p.second;
    session_statistic.items_.push_back(session->get_statistic());
  }
}

void vds::dht::network::udp_transport::add_session(
  const service_provider& sp,
  const network_address& address,
  const std::shared_ptr<dht_session>& session) {

  sp.get<logger>()->trace(ThisModule, sp, "Add session %s", address.to_string().c_str());

  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[address] = session;
  lock.unlock();

  (*sp.get<client>())->add_session(sp, session, 0);
}

std::shared_ptr<vds::dht::network::dht_session> vds::dht::network::udp_transport::get_session(
  const network_address& address) const {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  auto p = this->sessions_.find(address);
  if (this->sessions_.end() == p) {
    return std::shared_ptr<dht_session>();
  }

  return p->second;
}

std::future<void> vds::dht::network::udp_transport::continue_read(
  const service_provider& sp) {
  for (;;) {
    udp_datagram datagram = co_await this->server_.socket().read_async();

    if (sp.get_shutdown_event().is_shuting_down()) {
      co_return;
    }

    switch ((protocol_message_type_t)datagram.data()[0]) {
    case protocol_message_type_t::HandshakeBroadcast:
    case protocol_message_type_t::Handshake: {

      std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
      if (this->sessions_.end() != this->sessions_.find(datagram.address())) {
        lock.unlock();

        continue;
      }
      lock.unlock();

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
          break;
        }

        const_data_buffer key;
        key.resize(32);
        crypto_service::rand_bytes(key.data(), key.size());

        this->add_session(
          sp,
          datagram.address(),
          std::make_shared<dht_session>(
            datagram.address(),
            this->this_node_id_,
            partner_node_id,
            key));

        resizable_data_buffer out_message;
        out_message.add(static_cast<uint8_t>(protocol_message_type_t::Welcome));
        out_message.add(MAGIC_LABEL >> 24);
        out_message.add(MAGIC_LABEL >> 16);
        out_message.add(MAGIC_LABEL >> 8);
        out_message.add(MAGIC_LABEL);

        binary_serializer bs;
        bs
          << this->node_cert_.der()
          << partner_node_cert.public_key().encrypt(key);

        out_message += bs.move_data();
        co_await this->write_async(sp, udp_datagram(datagram.address(), out_message.move_data(), false));
        co_await (*sp.get<client>())->send_neighbors(
          sp,
          messages::dht_find_node_response({
          messages::dht_find_node_response::target_node(partner_node_id, datagram.address().to_string(), 0) }));

        co_await sp.get<imessage_map>()->on_new_session(
          sp,
          partner_node_id);
      }
      else {
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
        auto key = this->node_key_.decrypt(key_buffer);

        auto partner_id = cert.fingerprint(hash::sha256());

        //TODO: validate cert

        this->add_session(
          sp,
          datagram.address(),
          std::make_shared<dht_session>(
            datagram.address(),
            this->this_node_id_,
            partner_id,
            key));

        sp.get<imessage_map>()->on_new_session(
          sp,
          partner_id);
      }
      else {
        throw std::runtime_error("Invalid protocol");
      }

      break;
    }
    case protocol_message_type_t::Failed: {
      auto session = this->get_session(datagram.address());
      if (session) {
        std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
        this->sessions_.erase(datagram.address());
      }

      break;
    }
    default: {
      auto session = this->get_session(datagram.address());
      if (session) {
        try {
          auto scope = sp.create_scope("Process datagram");
          bool failed = false;
          try {
            co_await session->process_datagram(
              scope,
              this->shared_from_this(),
              const_data_buffer(datagram.data(), datagram.data_size()));
          }
          catch (const std::exception & /*ex*/) {
            failed = true;
          }

          if(failed) {
            uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
            co_await this->write_async(sp, udp_datagram(datagram.address(),
              const_data_buffer(out_message, sizeof(out_message))));

            std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
            this->sessions_.erase(datagram.address());
          }
        }
        catch (...) {
          std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
          this->sessions_.erase(datagram.address());
        }
      }
      else {
        uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
        auto scope = sp.create_scope("Send Failed");
        try {
          co_await this->write_async(scope, udp_datagram(datagram.address(),
            const_data_buffer(out_message, sizeof(out_message))));
        }
        catch (...) {
          std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
          this->sessions_.erase(datagram.address());
          this->continue_read(sp);
        }

      }
      break;
    }
    }
  }
}

