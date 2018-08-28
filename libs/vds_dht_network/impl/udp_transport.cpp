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

vds::dht::network::udp_transport::udp_transport()
: write_in_progress_(false) {
}

void vds::dht::network::udp_transport::start(
  const service_provider& sp,
  const certificate & node_cert,
  const asymmetric_private_key & node_key,
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

vds::async_task<>
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

  return [sp, pthis = this->shared_from_this(), datagram](const async_result<>& result) {
    auto this_ = static_cast<udp_transport *>(pthis.get());
    std::unique_lock<std::debug_mutex> lock(this_->write_mutex_);
    bool need_send = this_->send_queue_.empty();
    this_->send_queue_.push_back(std::make_tuple(datagram, std::move(result)));

    if (need_send) {
      sp.get<logger>()->trace(
        ThisModule,
        sp,
        "started send");
      this_->continue_send(sp);
    }
  };
}

void
vds::dht::network::udp_transport::continue_send(const service_provider& sp) {

  this->server_.socket().write_async(std::get<0>(this->send_queue_.front()))
      .execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception>& ex) {
        auto this_ = static_cast<udp_transport *>(pthis.get());
        sp.get<logger>()->trace(
          ThisModule,
          sp,
          "udp_transport socket sent");
        mt_service::async(sp, [sp, ex, pthis]() {
          auto this_ = static_cast<udp_transport *>(pthis.get());
          sp.get<logger>()->trace(
            ThisModule,
            sp,
            "udp_transport sent");

          std::unique_lock<std::debug_mutex> lock(this_->write_mutex_);
          auto p = this_->send_queue_.begin();
          auto datagram = std::move(std::get<0>(*p));
          auto result = std::move(std::get<1>(*p));
          this_->send_queue_.pop_front();
          if (!this_->send_queue_.empty()) {
            this_->continue_send(sp);
          }
          else {
            sp.get<logger>()->trace(
              ThisModule,
              sp,
              "stopped send");
          }
          lock.unlock();

          if (ex) {
            sp.get<logger>()->error(
              ThisModule,
              sp,
              "%s at write UDP datagram %d bytes to %s",
              ex->what(),
              datagram.data_size(),
              datagram.address().to_string().c_str());

            result.error(ex);
          }
          else {
            result.done();
          }
        });
      });
}


vds::async_task<> vds::dht::network::udp_transport::try_handshake(const service_provider& sp,
                                                                  const std::string& address) {

  this->block_list_mutex_.lock();
  auto p = this->block_list_.find(address);
  if (this->block_list_.end() != p && p->second < std::chrono::steady_clock::now()) {
    this->block_list_mutex_.unlock();
    return async_task<>::empty();
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

  return this->write_async(sp, udp_datagram(
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

void vds::dht::network::udp_transport::continue_read(
  const service_provider& sp) {
  if (!this->server_) {
    return;
  }
  this->server_.socket().read_async().execute([sp, pthis = this->shared_from_this()](
    const std::shared_ptr<std::exception>& ex,
    const udp_datagram& datagram) {
    auto this_ = static_cast<udp_transport *>(pthis.get());

      if (sp.get_shutdown_event().is_shuting_down()) {
        return;
      }

      if (!ex && 0 != datagram.data_size()) {
        switch ((protocol_message_type_t)datagram.data()[0]) {
        case protocol_message_type_t::HandshakeBroadcast:
        case protocol_message_type_t::Handshake: {

          std::unique_lock<std::shared_mutex> lock(this_->sessions_mutex_);
          if(this_->sessions_.end() != this_->sessions_.find(datagram.address())) {
            lock.unlock();

            this_->continue_read(sp);
            return;
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
            if (partner_node_id == this_->this_node_id_) {
              break;
            }

            const_data_buffer key;
            key.resize(32);
            crypto_service::rand_bytes(key.data(), key.size());

            this_->add_session(
              sp,
              datagram.address(),
              std::make_shared<dht_session>(
                datagram.address(),
                this_->this_node_id_,
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
                << this_->node_cert_.der()
                << partner_node_cert.public_key().encrypt(key);

            out_message += bs.move_data();
            pthis->write_async(sp, udp_datagram(datagram.address(), out_message.move_data(), false))
                 .execute([
                     pthis,
                     sp,
                     scope = sp.create_scope("Send Welcome"),
                     partner_node_id,
                     address = datagram.address().to_string()](const std::shared_ptr<std::exception>& ex) {
                       auto this_ = static_cast<udp_transport *>(pthis.get());
                     if (ex) {
                       sp.get<logger>()->trace(ThisModule, sp, "%s at send welcome to %s", ex->what(),
                                               address.c_str());
                     }
                     (*sp.get<client>())->send_neighbors(
                       sp,
                       messages::dht_find_node_response({
                       messages::dht_find_node_response::target_node(partner_node_id, address, 0) }));

                     sp.get<imessage_map>()->on_new_session(
                       sp,
                       partner_node_id);

                     this_->continue_read(sp);
                   });

            return;
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
            auto key = this_->node_key_.decrypt(key_buffer);

            auto partner_id = cert.fingerprint(hash::sha256());

            //TODO: validate cert

            this_->add_session(
              sp,
              datagram.address(),
              std::make_shared<dht_session>(
                datagram.address(),
                this_->this_node_id_,
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
          auto session = this_->get_session(datagram.address());
          if (session) {
            std::shared_lock<std::shared_mutex> lock(this_->sessions_mutex_);
            this_->sessions_.erase(datagram.address());
          }

          break;
        }
        default: {
          auto session = this_->get_session(datagram.address());
          if (session) {
            try {
              auto scope = sp.create_scope("Process datagram");
              session->process_datagram(scope, pthis, const_data_buffer(datagram.data(), datagram.data_size()))
                     .execute([pthis, sp, scope, datagram](
                       const std::shared_ptr<std::exception>& ex) {
                          auto this_ = static_cast<udp_transport *>(pthis.get());
                         if (ex) {
                           uint8_t out_message[] = { (uint8_t)protocol_message_type_t::Failed };
                           pthis->write_async(sp, udp_datagram(datagram.address(),
                             const_data_buffer(out_message, sizeof(out_message))))
                             .execute([pthis, sp, datagram](
                               const std::shared_ptr<std::exception>& ex) {
                             auto this_ = static_cast<udp_transport *>(pthis.get());
                             if (ex) {
                               sp.get<logger>()->trace(ThisModule, sp, "%s at send failed to %s", ex->what(),
                                 datagram.address().to_string().c_str());
                             }
                             std::shared_lock<std::shared_mutex> lock(this_->sessions_mutex_);
                             this_->sessions_.erase(datagram.address());
                             this_->continue_read(sp);
                           });
                         }
                         else {
                           mt_service::async(sp, [sp, pthis]() {
                             auto this_ = static_cast<udp_transport *>(pthis.get());
                             this_->continue_read(sp);
                           });
                         }
                       });
              return;
            }
            catch (...) {
              std::shared_lock<std::shared_mutex> lock(this_->sessions_mutex_);
              this_->sessions_.erase(datagram.address());
            }
          }
          else {
            uint8_t out_message[] = {(uint8_t)protocol_message_type_t::Failed};
            auto scope = sp.create_scope("Send Failed");
            pthis->write_async(scope, udp_datagram(datagram.address(),
                                                   const_data_buffer(out_message, sizeof(out_message))))
                 .execute([pthis, sp, scope, address = datagram.address()](
                   const std::shared_ptr<std::exception>& ex) {
                    auto this_ = static_cast<udp_transport *>(pthis.get());
                     if (ex) {
                       sp.get<logger>()->trace(ThisModule, sp, "%s at send failed to %s", ex->what(),
                                               address.to_string().c_str());
                     }
                     std::shared_lock<std::shared_mutex> lock(this_->sessions_mutex_);
                     this_->sessions_.erase(address);
                     this_->continue_read(sp);
                   });

            return;
          }
          break;
        }
        }
        this_->continue_read(sp);
      }
      else {
        if (ex) {
          sp.get<logger>()->trace(ThisModule, sp, "Error %s", ex->what());

          //auto sys_error = std::dynamic_pointer_cast<std::system_error>(ex);
          //if(sys_error && sys_error->code().value() == ERROR_PORT_UNREACHABLE) {
          //  return;          
          //}
        }
        if (0 == datagram.data_size()) {
          sp.get<logger>()->trace(ThisModule, sp, "0 == datagram.data_size()");
        }
        mt_service::async(sp, [sp, pthis]() {
          auto this_ = static_cast<udp_transport *>(pthis.get());
          if (!sp.get_shutdown_event().is_shuting_down()) {
            this_->continue_read(sp);
          }
        });
      }
    });
}

