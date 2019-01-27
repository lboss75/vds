#ifndef __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
#define __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <queue>

#include "udp_socket.h"
#include "const_data_buffer.h"
#include "udp_datagram_size_exception.h"
#include "vds_debug.h"
#include "messages/dht_route_messages.h"
#include "network_address.h"
#include "debug_mutex.h"
#include "vds_exceptions.h"
#include "hash.h"


namespace vds {
  namespace dht {
    namespace network {

      template <typename implementation_class, typename transport_type>
      class dht_datagram_protocol : public std::enable_shared_from_this<implementation_class> {
      public:
        static constexpr int CHECK_MTU_TIMEOUT = 10;

        dht_datagram_protocol(
          const service_provider * sp,
          const network_address& address,
          const const_data_buffer& this_node_id,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key) noexcept
          : sp_(sp),
          check_mtu_(0),
          address_(address),
          this_node_id_(this_node_id),
          partner_node_id_(partner_node_id),
          session_key_(session_key),
          mtu_(508),
          next_sequence_number_(0) {
        }

        void set_mtu(uint16_t value) {
          this->mtu_ = value;
        }

        vds::async_task<vds::expected<void>> send_message(
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          expected<const_data_buffer> && message) {
          CHECK_EXPECTED_ERROR(message);
          return this->send_message(s, message_type, target_node, message.value());
        }
          vds::async_task<vds::expected<void>> send_message(
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const const_data_buffer& message) {
          vds_assert(message.size() <= 0xFFFFFFFF);
          vds_assert(target_node != this->this_node_id_);

          logger::get(this->sp_)->trace(
            "dht_session",
            "send %d from this node %s to %s",
            message_type,
            base64::from_bytes(this->this_node_id_).c_str(),
            base64::from_bytes(target_node).c_str());

          return this->send_message_async(
            s,
            message_type,
            target_node,
            this->this_node_id_,
            0,
            message);
        }

        vds::async_task<vds::expected<void>> proxy_message(
          
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer& target_node,
          const const_data_buffer& source_node,
          const uint16_t hops,
          const const_data_buffer& message) {
          vds_assert(message.size() <= 0xFFFFFFFF);
          vds_assert(target_node != this->this_node_id_);
          vds_assert(source_node != this->partner_node_id_);

          logger::get(this->sp_)->trace(
            "dht_session",
            "send %d from %s to %s",
            message_type,
            base64::from_bytes(source_node).c_str(),
            base64::from_bytes(target_node).c_str());

          return this->send_message_async(
            s,
            message_type,
            target_node,
            source_node,
            hops,
            message);
        }

        vds::async_task<vds::expected<void>> process_datagram(
          
          const std::shared_ptr<transport_type>& s,
          const const_data_buffer& datagram) {

          if(protocol_message_type_t::MTUTest == static_cast<protocol_message_type_t>(*datagram.data())) {
            const_data_buffer data = datagram;
            data[0] = (uint8_t)protocol_message_type_t::MTUTestPassed;
            co_return co_await s->write_async(udp_datagram(this->address_, data));
          }
          if (protocol_message_type_t::MTUTestPassed == static_cast<protocol_message_type_t>(*datagram.data())) {
            if(this->mtu_ < datagram.size()) {
              this->mtu_ = datagram.size();
              this->check_mtu_ = 0;
              logger::get(this->sp_)->trace("dht_session", "Change MTU size to %d", this->mtu_);
            }
            co_return expected<void>();
          }

          this->input_mutex_.lock();
          if (datagram.size() < 33) {
            this->input_mutex_.unlock();
            co_return vds::make_unexpected<std::runtime_error>("Invalid data");
          }

          if (!hmac::verify(
            this->session_key_,
            hash::sha256(),
            datagram.data(), datagram.size() - 32,
            datagram.data() + datagram.size() - 32, 32)) {
            this->input_mutex_.unlock();

            co_return vds::make_unexpected<std::runtime_error>("Invalid signature");
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
            default:
              vds_assert(false);
              break;
            }
            }

            this->input_mutex_.unlock();

            CHECK_EXPECTED_ASYNC(co_await static_cast<implementation_class *>(this)->process_message(
              s,
              message_type,
              target_node,
              source_node,
              hops,
              message));
            co_return expected<void>();
          }

          case protocol_message_type_t::Data:
          case protocol_message_type_t::RouteData:
          case protocol_message_type_t::ProxyData: {
            if (datagram.size() < 33) {
              this->input_mutex_.unlock();

              co_return vds::make_unexpected<std::runtime_error>("Invalid data");
            }

            this->last_input_message_id_ = const_data_buffer(datagram.data() + 1, 32);
            this->input_messages_.clear();
            this->input_messages_[0] = datagram;
            this->input_mutex_.unlock();

            co_return expected<void>();
          }
          default: {
            if (datagram.data()[0] == (uint8_t)protocol_message_type_t::ContinueData) {
              if (datagram.size() < 1 + 32 + INDEX_SIZE) {
                this->input_mutex_.unlock();

                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }

              const auto message_id = const_data_buffer(datagram.data() + 1, 32);
              if (this->last_input_message_id_ != message_id) {
                this->last_input_message_id_.clear();
                this->input_messages_.clear();
                this->input_mutex_.unlock();

                co_return expected<void>();
              }
              uint32_t index =
                  (datagram.data()[1 + 32] << 24)
                | (datagram.data()[1 + 32 + 1] << 16)
                | (datagram.data()[1 + 32 + 2] << 8)
                | (datagram.data()[1 + 32 + 3]);
              this->input_messages_[index] = datagram;

              CHECK_EXPECTED_ASYNC(co_await this->continue_process_messages(s));
              co_return expected<void>();
            }
            else {
              this->input_mutex_.unlock();
              co_return vds::make_unexpected<std::runtime_error>("Invalid data");
            }
          }
          }
          this->input_mutex_.unlock();
          co_return expected<void>();
        }

        const network_address& address() const {
          return this->address_;
        }

        const const_data_buffer& this_node_id() const {
          return this->this_node_id_;
        }

        const const_data_buffer& partner_node_id() const {
          return this->partner_node_id_;
        }

        vds::async_task<vds::expected<void>> on_timer(
          const std::shared_ptr<transport_type>& s) {
          this->check_mtu_++;
          if (this->check_mtu_ < CHECK_MTU_TIMEOUT) {
            if (this->mtu_ < 0xFFFF - 256) {
              resizable_data_buffer out_message;
              out_message.resize_data(this->mtu_ + 256);
              out_message.add((uint8_t)protocol_message_type_t::MTUTest);

              (void)co_await s->write_async(udp_datagram(this->address_, out_message.move_data()));
            }
          }
          else if(this->mtu_ > 1024) {
            this->mtu_ -= 256;
            this->check_mtu_ = 0;
          }

          co_return expected<void>();
        }


      protected:
        const service_provider * sp_;

      private:
        static constexpr uint8_t INDEX_SIZE = 4;
        static constexpr uint8_t SIZE_SIZE = 4;

        struct send_queue_item_t {
          std::shared_ptr<transport_type> transport;
          uint8_t message_type;
          const_data_buffer target_node;
          const_data_buffer source_node;
          uint16_t hops;
          const_data_buffer message;
        };
        int check_mtu_;
        network_address address_;
        const_data_buffer this_node_id_;
        const_data_buffer partner_node_id_;
        const_data_buffer session_key_;

        uint16_t mtu_;

        not_mutex input_mutex_;
        const_data_buffer last_input_message_id_;
        std::map<uint32_t, const_data_buffer> input_messages_;
        uint32_t next_sequence_number_;


        vds::async_task<vds::expected<void>> send_message_async(
          
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
              GET_EXPECTED_ASYNC(sig, hmac::signature(
                this->session_key_,
                hash::sha256(),
                buffer.data(),
                buffer.size()));
              buffer += sig;

              const_data_buffer datagram = buffer.move_data();
              auto write_result = co_await s->write_async(udp_datagram(this->address_, datagram));
              if(write_result.has_error()) {
                auto error = dynamic_cast<udp_datagram_size_exception *>(write_result.error().get());
                if(nullptr != error) {
                  this->mtu_ /= 2;
                  continue;;
                }
                co_return unexpected(std::move(write_result.error()));
              }
            }
            else {
              binary_serializer bs;
              CHECK_EXPECTED_ASYNC(bs << message_type);
              CHECK_EXPECTED_ASYNC(bs << target_node);
                CHECK_EXPECTED_ASYNC(bs << source_node);
                  CHECK_EXPECTED_ASYNC(bs << hops);
                    CHECK_EXPECTED_ASYNC(bs <<  message);

              GET_EXPECTED_ASYNC(message_id, hash::signature(hash::sha256(), bs.move_data()));
              vds_assert(message_id.size() == 32);

              uint32_t offset;

              if (this->this_node_id_ == source_node) {
                if (this->partner_node_id_ == target_node) {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::Data | message_type));//1
                  buffer += message_id;//32
                  buffer.add((uint8_t)((message.size()) >> 24));//1
                  buffer.add((uint8_t)((message.size()) >> 16));//1
                  buffer.add((uint8_t)((message.size()) >> 8));//1
                  buffer.add((uint8_t)((message.size()) & 0xFF));//1
                  buffer.add(message.data(), this->mtu_ - (1 + 32 + SIZE_SIZE));
                  offset = this->mtu_ - (1 + 32 + SIZE_SIZE);
                }
                else {
                  buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteData | message_type));//1
                  buffer += message_id;//32
                  buffer.add((uint8_t)((message.size()) >> 24));//1
                  buffer.add((uint8_t)((message.size()) >> 16));//1
                  buffer.add((uint8_t)((message.size()) >> 8));//1
                  buffer.add((uint8_t)((message.size()) & 0xFF));//1
                  vds_assert(target_node.size() == 32);
                  buffer += target_node;//32
                  buffer.add(message.data(), this->mtu_ - (1 + 32 + SIZE_SIZE + 32));
                  offset = this->mtu_ - (1 + 32 + SIZE_SIZE + 32);
                }
              }
              else {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxyData | message_type));//1
                buffer += message_id;//32
                buffer.add((uint8_t)((message.size()) >> 24));//1
                buffer.add((uint8_t)((message.size()) >> 16));//1
                buffer.add((uint8_t)((message.size()) >> 8));//1
                buffer.add((uint8_t)((message.size()) & 0xFF));//1
                vds_assert(target_node.size() == 32);
                vds_assert(source_node.size() == 32);
                buffer += target_node;//32
                buffer += source_node;//32
                buffer.add((uint8_t)(hops));//1
                buffer.add(message.data(), this->mtu_ - (1 + 32 + SIZE_SIZE + 32 + 32 + 1));
                offset = this->mtu_ - (1 + 32 + SIZE_SIZE + 32 + 32 + 1);
              }

              GET_EXPECTED_ASYNC(sig, hmac::signature(
                this->session_key_,
                hash::sha256(),
                buffer.data(),
                buffer.size()));

              buffer += sig;
              const_data_buffer datagram = buffer.move_data();
              auto write_result = co_await s->write_async(udp_datagram(this->address_, datagram));
              if (write_result.has_error()) {
                auto error = dynamic_cast<udp_datagram_size_exception *>(write_result.error().get());
                if (nullptr != error) {
                  this->mtu_ /= 2;
                  continue;;
                }
                co_return unexpected(std::move(write_result.error()));
              }

              uint32_t index = 1;
              for (;;) {
                auto size = this->mtu_ - (1 + 32 + INDEX_SIZE + 32);
                if (size > message.size() - offset) {
                  size = message.size() - offset;
                }

                resizable_data_buffer buffer;
                buffer.add((uint8_t)protocol_message_type_t::ContinueData);//1
                buffer += message_id;//32
                buffer.add(index >> 24);//1
                buffer.add(index >> 16);//1
                buffer.add(index >> 8);//1
                buffer.add(index);//1
                buffer.add(message.data() + offset, size);//

                GET_EXPECTED_ASYNC(sig, hmac::signature(
                  this->session_key_,
                  hash::sha256(),
                  buffer.data(),
                  buffer.size()));
                buffer += sig;

                const_data_buffer datagram = buffer.move_data();
                auto write_result = co_await s->write_async(udp_datagram(this->address_, datagram));
                if (write_result.has_error()) {
                  auto error = dynamic_cast<udp_datagram_size_exception *>(write_result.error().get());
                  if (nullptr != error) {
                    this->mtu_ /= 2;
                    continue;;
                  }
                  co_return unexpected(std::move(write_result.error()));
                }

                if (offset + size >= message.size()) {
                  break;
                }
                offset += size;
                ++index;
                vds_assert(index < 0xFFFFFF);
              }
            }

            break;
          }

          co_return expected<void>();
        }

        vds::async_task<vds::expected<void>> continue_process_messages(

          const std::shared_ptr<transport_type>& s) {
          auto p = this->input_messages_.find(0);
          if (p == this->input_messages_.end()) {
            this->input_mutex_.unlock();

            co_return expected<void>();
          }

          switch (
            static_cast<protocol_message_type_t>(
            (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
            ) {
          case protocol_message_type_t::Data:
          case protocol_message_type_t::RouteData:
          case protocol_message_type_t::ProxyData: {
            auto message_type = p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand;
            size_t size =
                          uint32_t(p->second.data()[1 + 32] << 24)
                        | uint32_t(p->second.data()[1 + 32 + 1] << 16)
                        | uint32_t(p->second.data()[1 + 32 + 2] << 8)
                        | uint32_t(p->second.data()[1 + 32 + 3]);

            const_data_buffer target_node;
            const_data_buffer source_node;
            uint16_t hops;
            resizable_data_buffer message;

            switch (
              static_cast<protocol_message_type_t>(
              (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
              ) {
            case protocol_message_type_t::Data: {
              if (size <= p->second.size() - (1 + 32 + SIZE_SIZE + 32)) {
                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }
              vds_assert(p->second.size() > (1 + 32 + SIZE_SIZE + 32));
              size -= p->second.size() - (1 + 32 + SIZE_SIZE + 32);

              target_node = this->this_node_id_;
              source_node = this->partner_node_id_;
              hops = 0;
              message.add(p->second.data() + 1 + 32 + SIZE_SIZE, p->second.size() - (1 + 32 + SIZE_SIZE + 32));
              break;
            }

            case protocol_message_type_t::RouteData: {
              if (size <= p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32)) {
                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }
              vds_assert(p->second.size() > (1 + 32 + SIZE_SIZE + 32 + 32));
              size -= p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32);

              target_node = const_data_buffer(p->second.data() + 1 + 32 + SIZE_SIZE, 32);
              source_node = this->partner_node_id_;
              hops = 0;
              message.add(p->second.data() + (1 + 32 + SIZE_SIZE + 32), p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32));
              break;
            }

            case protocol_message_type_t::ProxyData: {
              if (size <= p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32 + 1 + 32)) {
                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }
              vds_assert(p->second.size() > (1 + 32 + SIZE_SIZE + 32 + 32 + 1 + 32));
              size -= p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32 + 1 + 32);

              target_node = const_data_buffer(p->second.data() + 1 + 32 + SIZE_SIZE, 32);
              source_node = const_data_buffer(p->second.data() + 1 + 32 + SIZE_SIZE + 32, 32);

              hops = p->second.data()[1 + 32 + SIZE_SIZE + 32 + 32];
              message.add(p->second.data() + (1 + 32 + SIZE_SIZE + 32 + 32 + 1), p->second.size() - (1 + 32 + SIZE_SIZE + 32 + 32 + 1 + 32));
              break;
            }
            default:
              co_return vds::make_unexpected<std::runtime_error>("Invalid program");
            }

            for (uint32_t index = 1;; ++index) {
              auto p1 = this->input_messages_.find(index);
              if (this->input_messages_.end() == p1) {
                break;
              }

              if ((uint8_t)protocol_message_type_t::ContinueData != p1->second.data()[0]) {
                this->input_mutex_.unlock();

                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }

              if (size < p1->second.size() - (1 + 32 + INDEX_SIZE + 32)) {
                this->input_mutex_.unlock();

                co_return vds::make_unexpected<std::runtime_error>("Invalid data");
              }

              message.add(p1->second.data() + (1 + 32 + INDEX_SIZE), p1->second.size() - (1 + 32 + INDEX_SIZE + 32));
              vds_assert(p1->second.size() > (1 + 32 + INDEX_SIZE + 32));
              size -= p1->second.size() - (1 + 32 + INDEX_SIZE + 32);

              if (0 == size) {
                this->last_input_message_id_.clear();
                this->input_messages_.clear();
                this->input_mutex_.unlock();

                CHECK_EXPECTED_ASYNC(co_await static_cast<implementation_class *>(this)->process_message(
                  s,
                  message_type,
                  target_node,
                  source_node,
                  hops,
                  message.move_data()));

                co_return expected<void>();
              }
            }

            break;
          }
          default: {
            vds_assert(false);
            break;
          }
          }
          this->input_mutex_.unlock();
          co_return expected<void>();
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
