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

  const auto na = network_address::parse(this->server_.address().family(), address);
  this->sessions_mutex_.lock();
  auto & session_info = this->sessions_[na];
  this->sessions_mutex_.unlock();

  std::unique_lock<std::mutex> lock(session_info.session_mutex_);
  if (session_info.blocked_) {
    if ((std::chrono::steady_clock::now() - session_info.update_time_) > std::chrono::minutes(1)) {
      session_info.blocked_ = false;
    }
    else {
      return async_task<>::empty();
    }
  }

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
    na,
    out_message.move_data(),
    false));
}

void vds::dht::network::udp_transport::get_session_statistics(session_statistic& session_statistic) {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for (const auto& p : this->sessions_) {
    session_statistic.items_.push_back({
      p.first.to_string(),
      p.second.blocked_,
      !p.second.session_
    });
  }
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
        this_->sessions_mutex_.lock();
        auto & session_info = this_->sessions_[datagram.address()];
        this_->sessions_mutex_.unlock();

        std::unique_lock<std::mutex> lock(session_info.session_mutex_);
        if (session_info.blocked_) {
          if ((std::chrono::steady_clock::now() - session_info.update_time_) > std::chrono::minutes(1)) {
            session_info.blocked_ = false;
          }
          else {
            this_->continue_read(sp);
            return;
          }
        }

        switch ((protocol_message_type_t)datagram.data()[0]) {
        case protocol_message_type_t::HandshakeBroadcast:
        case protocol_message_type_t::Handshake: {
          
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

            if (!session_info.session_key_ || (std::chrono::steady_clock::now() - session_info.update_time_) > std::chrono::minutes(10)) {
              session_info.update_time_ = std::chrono::steady_clock::now();
              session_info.session_key_.resize(32);
              crypto_service::rand_bytes(session_info.session_key_.data(), session_info.session_key_.size());
            }

            session_info.session_ = std::make_shared<dht_session>(
                datagram.address(),
                this_->this_node_id_,
                partner_node_id,
                session_info.session_key_);

            sp.get<logger>()->debug(ThisModule, sp, "Add session %s", datagram.address().to_string().c_str());
            (*sp.get<client>())->add_session(sp, session_info.session_, 0);

            resizable_data_buffer out_message;
            out_message.add(static_cast<uint8_t>(protocol_message_type_t::Welcome));
            out_message.add(MAGIC_LABEL >> 24);
            out_message.add(MAGIC_LABEL >> 16);
            out_message.add(MAGIC_LABEL >> 8);
            out_message.add(MAGIC_LABEL);

            binary_serializer bs;
            bs
                << this_->node_cert_.der()
                << partner_node_cert.public_key().encrypt(session_info.session_key_);

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

            session_info.session_ = std::make_shared<dht_session>(
                datagram.address(),
                this_->this_node_id_,
                partner_id,
                key);

            sp.get<logger>()->debug(ThisModule, sp, "Add session %s", datagram.address().to_string().c_str());
            (*sp.get<client>())->add_session(sp, session_info.session_, 0);

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
          session_info.blocked_ = true;
          session_info.session_.reset();
          session_info.update_time_ = std::chrono::steady_clock::now();
          break;
        }
        default: {
          if (session_info.session_) {
            try {
              auto scope = sp.create_scope("Process datagram");
              session_info.session_->process_datagram(scope, pthis, const_data_buffer(datagram.data(), datagram.data_size()))
                     .execute([pthis, sp, scope, datagram](
                       const std::shared_ptr<std::exception>& ex) {
                          auto this_ = static_cast<udp_transport *>(pthis.get());
                         if (ex) {
                           this_->sessions_mutex_.lock();
                           auto & session_info = this_->sessions_[datagram.address()];
                           this_->sessions_mutex_.unlock();
                           
                           session_info.session_mutex_.lock();
                           session_info.blocked_ = true;
                           session_info.session_.reset();
                           session_info.update_time_ = std::chrono::steady_clock::now();
                           session_info.session_mutex_.unlock();

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
              session_info.blocked_ = true;
              session_info.session_.reset();
              session_info.update_time_ = std::chrono::steady_clock::now();
            }
          }
          else {
            session_info.blocked_ = true;
            session_info.session_.reset();
            session_info.update_time_ = std::chrono::steady_clock::now();

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

                     mt_service::async(sp, [sp, pthis]() {
                       auto this_ = static_cast<udp_transport *>(pthis.get());
                       if (!sp.get_shutdown_event().is_shuting_down()) {
                         this_->continue_read(sp);
                       }
                     });
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

