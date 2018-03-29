/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "stdafx.h"
#include "private/udp_transport.h"
#include "private/dht_message_type.h"
#include "private/dht_session.h"
#include "logger.h"
#include "dht_network_client.h"
#include "dht_network_client_p.h"
#include "db_model.h"

void vds::dht::network::udp_transport::start(const vds::service_provider &sp, uint16_t port,
                                             const const_data_buffer &this_node_id) {
  this->this_node_id_ = this_node_id;

  try {
    this->server_.start(sp, network_address::any_ip6(port));
  }
  catch (...) {
    this->server_.start(sp, network_address::any_ip4(port));
  }

  this->continue_read(sp);
}

void vds::dht::network::udp_transport::stop(const service_provider& sp) {
  this->server_.stop(sp);
}

vds::async_task<> vds::dht::network::udp_transport::write_async(const udp_datagram& datagram) {
  std::unique_lock<std::debug_mutex> lock(this->write_mutex_);
  while(this->write_in_progress_) {
    this->write_cond_.wait(*reinterpret_cast<std::unique_lock<std::mutex> *>(&lock));
  }
  this->write_in_progress_ = true;

  return [pthis = this->shared_from_this(), datagram](const async_result<> & result) {
    pthis->server_.socket().write_async(datagram).execute([pthis, result](const std::shared_ptr<std::exception> & ex) {
      std::unique_lock<std::debug_mutex> lock(pthis->write_mutex_);
      pthis->write_in_progress_ = false;
      pthis->write_cond_.notify_all();
      lock.unlock();
      if(ex) {
        result.error(ex);
      }
      else {
        result.done();
      }
    });
  };
}

void vds::dht::network::udp_transport::on_timer(const service_provider& sp) {
  resizable_data_buffer out_message;
  out_message.add((uint8_t)protocol_message_type_t::HandshakeBroadcast);
  out_message.add(PROTOCOL_VERSION);
  out_message += this->this_node_id_;

  this->server_.socket().send_broadcast(8050, const_data_buffer(out_message.data(), out_message.size()));

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  for(auto & p : this->sessions_) {
    p.second->on_timer(sp, this->shared_from_this());
  }
}

void vds::dht::network::udp_transport::try_handshake(const service_provider& sp, const std::string& address) {
  resizable_data_buffer out_message;
  out_message.add((uint8_t)protocol_message_type_t::HandshakeBroadcast);
  out_message.add(PROTOCOL_VERSION);
  out_message += this->this_node_id_;

  this->write_async(
    udp_datagram(
      network_address::parse(address),
      const_data_buffer(out_message.data(), out_message.size()))).execute([](const std::shared_ptr<std::exception> & ex){
  });
}

void vds::dht::network::udp_transport::add_session(
  const vds::service_provider& sp,
  const network_address& address,
  const std::shared_ptr<dht_session>& session) {

  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[address] = session;
  lock.unlock();

  (*sp.get<client>())->add_session(sp, session, 0);
}

std::shared_ptr<vds::dht::network::dht_session> vds::dht::network::udp_transport::get_session(
  const network_address& address) const {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  auto p = this->sessions_.find(address);
  if(this->sessions_.end() == p) {
    return std::shared_ptr<dht_session>();
  }

  return p->second;
}

void vds::dht::network::udp_transport::continue_read(
  const vds::service_provider &sp) {

  this->server_.socket().read_async().execute([sp, pthis = this->shared_from_this()](
    const std::shared_ptr<std::exception> & ex,
    const vds::udp_datagram & datagram){
    if (!ex && 0 != datagram.data_size()) {
      switch ((protocol_message_type_t)datagram.data()[0]) {
      case protocol_message_type_t::HandshakeBroadcast: {
        auto session = pthis->get_session(datagram.address());
        if (session) {
          break;
        }
        //break;
      }
      case protocol_message_type_t::Handshake:
        if (datagram.data_size() == NODE_ID_SIZE + 2 && PROTOCOL_VERSION == datagram.data()[1]) {
          const_data_buffer partner_node_id(datagram.data() + 2, NODE_ID_SIZE);
          if(partner_node_id == pthis->this_node_id_){
            break;
          }

          pthis->add_session(
            sp,
            datagram.address(), std::make_shared<dht_session>(
              datagram.address(),
              pthis->this_node_id_,
              partner_node_id));

          resizable_data_buffer out_message;
          out_message.add((uint8_t)protocol_message_type_t::Welcome);
          out_message.add(pthis->this_node_id_.data(), pthis->this_node_id_.size());

          pthis->write_async(udp_datagram(datagram.address(), out_message.data(), out_message.size()))
            .execute([pthis, sp, address = datagram.address().to_string()](const std::shared_ptr<std::exception> & ex) {
            if (ex) {
              sp.get<logger>()->trace(ThisModule, sp, "%s at send welcome to %s", ex->what(), address.c_str());
            }
            pthis->continue_read(sp);
          });

          return;
        }
        break;
      case protocol_message_type_t::Welcome:
        if (datagram.data_size() == NODE_ID_SIZE + 1) {
          const_data_buffer partner_node_id(datagram.data() + 1, NODE_ID_SIZE);
          pthis->add_session(
            sp,
            datagram.address(), std::make_shared<dht_session>(
              datagram.address(),
              pthis->this_node_id_,
              partner_node_id));
        }
        break;
      case protocol_message_type_t::Failed: {
        auto session = pthis->get_session(datagram.address());
        if (session) {
          std::shared_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
          pthis->sessions_.erase(datagram.address());
        }

        break;
      }
      default: {
        auto session = pthis->get_session(datagram.address());
        if (session) {
          try {
            session->process_datagram(sp, pthis, const_data_buffer(datagram.data(), datagram.data_size()))
              .execute([pthis, sp, address = datagram.address()](const std::shared_ptr<std::exception> & ex){
              if (ex) {
                std::shared_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
                pthis->sessions_.erase(address);
              }
              pthis->continue_read(sp);
            });
            return;
          }
          catch(...) {
            std::shared_lock<std::shared_mutex> lock(pthis->sessions_mutex_);
            pthis->sessions_.erase(datagram.address());
          }
        }
        else {
          resizable_data_buffer out_message;
          out_message.add((uint8_t)protocol_message_type_t::Failed);

          pthis->write_async(udp_datagram(datagram.address(), out_message.data(), out_message.size()))
            .execute([pthis, sp, address = datagram.address().to_string()](const std::shared_ptr<std::exception> & ex) {
            if (ex) {
              sp.get<logger>()->trace(ThisModule, sp, "%s at send welcome to %s", ex->what(), address.c_str());
            }
            pthis->continue_read(sp);
          });

          return;
        }
        break;
      }
      }
      pthis->continue_read(sp);
    }
    else {
      mt_service::async(sp, [sp, pthis]() {
        pthis->continue_read(sp);
      });
    }
  });
}

vds::dht::network::udp_transport::udp_transport()
: write_in_progress_(false){
}

