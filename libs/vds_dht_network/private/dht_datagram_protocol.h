#ifndef __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
#define __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <udp_socket.h>

#include "const_data_buffer.h"
#include "udp_datagram_size_exception.h"
#include "vds_debug.h"
#include "messages/dht_message_type.h"
#include "network_address.h"
#include "debug_mutex.h"
#include <queue>
#include "vds_exceptions.h"
#include "hash.h"


namespace vds {
  namespace dht {
    namespace network {

      template <typename implementation_class, typename transport_type>
      class dht_datagram_protocol : public std::enable_shared_from_this<implementation_class> {
      public:
        dht_datagram_protocol(
          const network_address& address,
          const const_data_buffer& this_node_id,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key)
          : address_(address),
            this_node_id_(this_node_id),
            partner_node_id_(partner_node_id),
            session_key_(session_key),
            mtu_(65507),
            next_sequence_number_(0) {
        }

        void set_mtu(uint16_t value){
          this->mtu_ = value;
        }

        std::future<void> send_message(
            const service_provider& sp,
            const std::shared_ptr<transport_type>& s,
            uint8_t message_type,
            const const_data_buffer& target_node,
            const const_data_buffer& message) {
          vds_assert(message.size() <= 0xFFFF);
          vds_assert(target_node != this->this_node_id_);

          sp.get<logger>()->trace(
              "dht_session",
              sp,
              "send %d from this node %s to %s",
              message_type,
              base64::from_bytes(this->this_node_id_).c_str(),
              base64::from_bytes(target_node).c_str());

          co_await this->send_message_async(
            sp,
            s,
            message_type,
            target_node,
            this->this_node_id_,
            0,
            message);
        }

        std::future<void> proxy_message(
          const service_provider& sp,
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const const_data_buffer& source_node,
          const uint16_t hops,
          const const_data_buffer& message) {
          vds_assert(message.size() <= 0xFFFF);
          vds_assert(target_node != this->this_node_id_);
          vds_assert(source_node != this->partner_node_id_);

          sp.get<logger>()->trace(
            "dht_session",
            sp,
            "send %d from %s to %s",
            message_type,
            base64::from_bytes(source_node).c_str(),
            base64::from_bytes(target_node).c_str());

          co_await this->send_message_async(
            sp,
            s,
            message_type,
            target_node,
            source_node,
            hops,
            message);
        }

        std::future<void> process_datagram(
          const service_provider& scope,
          const std::shared_ptr<transport_type>& s,
          const const_data_buffer& datagram) {

          auto sp = scope.create_scope(__FUNCTION__);
          std::unique_lock<std::mutex> lock(this->input_mutex_);
          if (datagram.size() < 33) {
            throw std::runtime_error("Invalid data");
          }

          if (!hmac::verify(
            this->session_key_,
            hash::sha256(),
            datagram.data(), datagram.size() - 32,
            datagram.data() + datagram.size() - 32, 32)) {
            throw std::runtime_error("Invalid signature");
          }

          switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & *datagram.data())) {
          case protocol_message_type_t::SingleData:
          case protocol_message_type_t::RouteSingleData:
          case protocol_message_type_t::ProxySingleData: {
            auto message_type = (uint8_t)(datagram.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand);

            const_data_buffer target_node;
            const_data_buffer source_node;
            uint16_t hops;
            const_data_buffer message;

            switch (
              static_cast<protocol_message_type_t>(
              (uint8_t)protocol_message_type_t::SpecialCommand & datagram.data()[0])
              ) {
            case protocol_message_type_t::SingleData: {
              target_node = this->this_node_id_;
              source_node = this->partner_node_id_;
              hops = 0;
              message = const_data_buffer(datagram.data() + 1, datagram.size() - 1 - 32);
              break;
            }

            case protocol_message_type_t::RouteSingleData: {
              target_node = const_data_buffer(datagram.data() + 1, 32);
              source_node = this->partner_node_id_;
              hops = 0;
              message = const_data_buffer(datagram.data() + 33, datagram.size() - 33 - 32);
              break;
            }

            case protocol_message_type_t::ProxySingleData: {
              target_node = const_data_buffer(datagram.data() + 1, 32);
              source_node = const_data_buffer(datagram.data() + 1 + 32, 32);
              hops = datagram.data()[1 + 32 + 32];

              message = const_data_buffer(datagram.data() + 1 + 32 + 32 + 1, datagram.size() - (1 + 32 + 32 + 1 + 32));
              break;
            }
            }

            co_await static_cast<implementation_class *>(this)->process_message(
              sp,
              s,
              message_type,
              target_node,
              source_node,
              hops,
              message);
            co_return;
          }

          case protocol_message_type_t::Data:
          case protocol_message_type_t::RouteData:
          case protocol_message_type_t::ProxyData: {
            if (datagram.size() < 33) {
              throw std::runtime_error("Invalid data");
            }

            this->last_input_message_id_ = const_data_buffer(datagram.data() + 1, 32);
            this->input_messages_.clear();
            this->input_messages_[0] = datagram;

            co_return;
          }
          default: {
            if (datagram.data()[0] == (uint8_t)protocol_message_type_t::ContinueData) {
              if (datagram.size() < 34) {
                throw std::runtime_error("Invalid data");
              }

              const auto message_id = const_data_buffer(datagram.data() + 1, 32);
              if (this->last_input_message_id_ != message_id) {
                throw std::runtime_error("Invalid message_id");
              }
              this->input_messages_[datagram.data()[1 + 32]] = datagram;
              co_await this->continue_process_messages(sp, s, lock);
            }
            else {
              throw std::runtime_error("Invalid data");
            }
          }
          }
        }

        const network_address& address() const {
          return this->address_;
        }

        const const_data_buffer this_node_id() const {
          return this->this_node_id_;
        }

        const const_data_buffer& partner_node_id() const {
          return this->partner_node_id_;
        }

      private:
        struct send_queue_item_t {
          service_provider sp;
          std::shared_ptr<transport_type> transport;
          uint8_t message_type;
          const_data_buffer target_node;
          const_data_buffer source_node;
          uint16_t hops;
          const_data_buffer message;
        };

        network_address address_;
        const_data_buffer this_node_id_;
        const_data_buffer partner_node_id_;
        const_data_buffer session_key_;

        uint32_t mtu_;

        std::mutex input_mutex_;
        const_data_buffer last_input_message_id_;
        std::map<uint32_t, const_data_buffer> input_messages_;
        uint32_t next_sequence_number_;


        std::future<void> send_message_async(
          const service_provider& sp,
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const const_data_buffer& source_node,
          const uint8_t hops,
          const const_data_buffer& message) {

          for (;;) {
            resizable_data_buffer buffer;

            if (message.size() < this->mtu_ - 5) {
              if (this->this_node_id_ == source_node) {
                if (this->partner_node_id_ == target_node) {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::SingleData | message_type));
                }
                else {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteSingleData | message_type));
                  buffer += target_node;
                }
              }
              else {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxySingleData | message_type));
                buffer += target_node;
                buffer += source_node;
                buffer.add(hops);
              }
              buffer += message;
              buffer += hmac::signature(
                this->session_key_,
                hash::sha256(),
                buffer.data(),
                buffer.size());

              const_data_buffer datagram = buffer.move_data();
              try {
                s->write_async(sp, udp_datagram(this->address_, datagram, false));
              }
              catch (const udp_datagram_size_exception & ex) {
                this->mtu_ /= 2;
                continue;;
              }
            }
            else {
              binary_serializer bs;
              bs
                << message_type
                << target_node
                << source_node
                << hops
                << message;
              const auto message_id = hash::signature(hash::sha256(), bs.move_data());
              vds_assert(message_id.size() == 32);

              uint16_t offset;

              if (this->this_node_id_ == source_node) {
                if (this->partner_node_id_ == target_node) {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::Data | message_type));//1
                  buffer += message_id;//32
                  buffer.add((uint8_t)((message.size()) >> 8));//1
                  buffer.add((uint8_t)((message.size()) & 0xFF));//1
                  buffer.add(message.data(), this->mtu_ - (1 + 32 + 2));
                  offset = this->mtu_ - (1 + 32 + 2);
                }
                else {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteData | message_type));//1
                  buffer += message_id;//32
                  buffer.add((uint8_t)((message.size()) >> 8));//1
                  buffer.add((uint8_t)((message.size()) & 0xFF));//1
                  vds_assert(target_node.size() == 32);
                  buffer += target_node;//32
                  buffer.add(message.data(), this->mtu_ - (1 + 32 + 2 + 32));
                  offset = this->mtu_ - (1 + 32 + 2 + 32);
                }
              }
              else {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxyData | message_type));//1
                buffer += message_id;//32
                buffer.add((uint8_t)((message.size()) >> 8));//1
                buffer.add((uint8_t)((message.size()) & 0xFF));//1
                vds_assert(target_node.size() == 32);
                vds_assert(source_node.size() == 32);
                buffer += target_node;//32
                buffer += source_node;//32
                buffer.add((uint8_t)(hops));//1
                buffer.add(message.data(), this->mtu_ - (1 + 32 + 2 + 32 + 32 + 1));
                offset = this->mtu_ - (1 + 32 + 2 + 32 + 32 + 1);
              }

              buffer += hmac::signature(
                this->session_key_,
                hash::sha256(),
                buffer.data(),
                buffer.size());
              const_data_buffer datagram = buffer.move_data();
              try {
                s->write_async(sp, udp_datagram(this->address_, datagram, false));
              }
              catch (const udp_datagram_size_exception & ) {
                this->mtu_ /= 2;
                continue;
              }

              uint8_t index = 1;
              for (;;) {
                auto size = this->mtu_ - (1 + 32 + 1 + 32);
                if (size > message.size() - offset) {
                  size = message.size() - offset;
                }

                resizable_data_buffer buffer;
                buffer.add((uint8_t)protocol_message_type_t::ContinueData);//1
                buffer += message_id;//32
                buffer.add(index);//1
                buffer.add(message.data() + offset, size);//

                buffer += hmac::signature(
                  this->session_key_,
                  hash::sha256(),
                  buffer.data(),
                  buffer.size());

                const_data_buffer datagram = buffer.move_data();
                try {
                  co_await s->write_async(sp, udp_datagram(this->address_, datagram, false));
                }
                catch (const udp_datagram_size_exception & ) {
                  this->mtu_ /= 2;
                  continue;
                }

                if (offset + size >= message.size()) {
                  break;
                }
                offset += size;
                ++index;
              }
            }

            break;
          }
        }

        std::future<void> continue_process_messages(
          const service_provider& sp,
          const std::shared_ptr<transport_type>& s,
          std::unique_lock<std::mutex>& locker
        ) {
          auto p = this->input_messages_.find(0);
          if (p == this->input_messages_.end()) {
            co_return;
          }

          switch (
            static_cast<protocol_message_type_t>(
            (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
            ) {
          case protocol_message_type_t::Data:
          case protocol_message_type_t::RouteData:
          case protocol_message_type_t::ProxyData: {
            auto message_type = p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand;
            size_t size = (p->second.data()[1 + 32] << 8) | (p->second.data()[1 + 32 + 1]);

            const_data_buffer target_node;
            const_data_buffer source_node;
            uint16_t hops;
            resizable_data_buffer message;

            switch (
              static_cast<protocol_message_type_t>(
              (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
              ) {
            case protocol_message_type_t::Data: {
              if (size <= p->second.size() - (1 + 32 + 2 + 32)) {
                throw std::runtime_error("Invalid data");
              }
              size -= p->second.size() - (1 + 32 + 2 + 32);

              target_node = this->this_node_id_;
              source_node = this->partner_node_id_;
              hops = 0;
              message.add(p->second.data() + 1 + 32 + 2, p->second.size() - (1 + 32 + 2 + 32));
              break;
            }

            case protocol_message_type_t::RouteData: {
              if (size <= p->second.size() - (1 + 32 + 2 + 32 + 32)) {
                throw std::runtime_error("Invalid data");
              }
              size -= p->second.size() - (1 + 32 + 2 + 32 + 32);

              target_node = const_data_buffer(p->second.data() + 1 + 32 + 2, 32);
              source_node = this->partner_node_id_;
              hops = 0;
              message.add(p->second.data() + (1 + 32 + 2 + 32), p->second.size() - (1 + 32 + 2 + 32 + 32));
              break;
            }

            case protocol_message_type_t::ProxyData: {
              if (size <= p->second.size() - (1 + 32 + 2 + 32 + 32 + 1 + 32)) {
                throw std::runtime_error("Invalid data");
              }
              size -= p->second.size() - (1 + 32 + 2 + 32 + 32 + 1 + 32);

              target_node = const_data_buffer(p->second.data() + 1 + 32 + 2, 32);
              source_node = const_data_buffer(p->second.data() + 1 + 32 + 2 + 32, 32);

              hops = p->second.data()[1 + 32 + 2 + 32 + 32];
              message.add(p->second.data() + (1 + 32 + 2 + 32 + 32 + 1), p->second.size() - (1 + 32 + 2 + 32 + 32 + 1 + 32));
              break;
            }
            }

            for (uint8_t index = 1;; ++index) {
              auto p1 = this->input_messages_.find(index);
              if (this->input_messages_.end() == p1) {
                break;
              }

              if ((uint8_t)protocol_message_type_t::ContinueData != p1->second.data()[0]) {
                throw std::runtime_error("Invalid data");
              }

              if (size < p1->second.size() - (1 + 32 + 1 + 32)) {
                throw std::runtime_error("Invalid data");
              }

              message.add(p1->second.data() + (1 + 32 + 1), p1->second.size() - (1 + 32 + 1 + 32));
              size -= p1->second.size() - (1 + 32 + 1 + 32);

              if (0 == size) {
                this->last_input_message_id_.clear();
                this->input_messages_.clear();
                locker.unlock();

                co_await static_cast<implementation_class *>(this)->process_message(
                  sp,
                  s,
                  message_type,
                  target_node,
                  source_node,
                  hops,
                  message.move_data());

                co_return;
              }
            }

            break;
          }
          default: {
            vds_assert(false);
            break;
          }
          }
        }

      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
