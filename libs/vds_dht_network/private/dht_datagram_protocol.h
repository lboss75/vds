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
#include "session_statistic.h"

namespace vds {
  namespace dht {
    namespace network {

      template <typename implementation_class, typename transport_type>
      class dht_datagram_protocol : public std::enable_shared_from_this<implementation_class> {
      public:
        static constexpr int CHECK_MTU_TIMEOUT = 10;
        static constexpr int MIN_MTU = 508;

        dht_datagram_protocol(
          const service_provider * sp,
          const network_address& address,
          const const_data_buffer& this_node_id,
          const const_data_buffer& partner_node_id,
          const const_data_buffer& session_key) noexcept
          : sp_(sp),
          failed_state_(false),
          check_mtu_(0),
          last_sent_(0),
          address_(address),
          this_node_id_(this_node_id),
          partner_node_id_(partner_node_id),
          session_key_(session_key),
          mtu_(MIN_MTU),
          input_mtu_(0),
          last_output_index_(0),
          last_input_index_(0) {
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
          vds_assert(0 == (message_type & static_cast<uint8_t>(protocol_message_type_t::SpecialCommand)));

          logger::get(this->sp_)->trace(
            "dht_session",
            "send %d from this node %s to %s",
            message_type,
            base64::from_bytes(this->this_node_id_).c_str(),
            base64::from_bytes(target_node).c_str());

          std::unique_lock<std::mutex> lock(this->traffic_mutex_);
          this->traffic_[base64::from_bytes(this->this_node_id_)][base64::from_bytes(target_node)][message_type].sent_count_++;
          this->traffic_[base64::from_bytes(this->this_node_id_)][base64::from_bytes(target_node)][message_type].sent_ += message.size();
          lock.unlock();

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
          vds_assert(0 == (message_type & static_cast<uint8_t>(protocol_message_type_t::SpecialCommand)));

          logger::get(this->sp_)->trace(
            "dht_session",
            "send %d from %s to %s",
            message_type,
            base64::from_bytes(source_node).c_str(),
            base64::from_bytes(target_node).c_str());

          std::unique_lock<std::mutex> lock(this->traffic_mutex_);
          this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].sent_count_++;
          this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].sent_ += message.size();
          lock.unlock();

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

          if (this->failed_state_) {
            co_return make_unexpected<std::runtime_error>("failed state");
          }

          if (this->last_sent_ > SEND_TIMEOUT) {
            co_return make_unexpected<std::runtime_error>("Send timeout");
          }

          if (this->input_mtu_ < datagram.size()) {
            this->input_mtu_ = datagram.size();
          }

          if(protocol_message_type_t::MTUTest == static_cast<protocol_message_type_t>(*datagram.data())) {
            co_return vds::expected<void>();
          }

          if (protocol_message_type_t::Acknowledgment == static_cast<protocol_message_type_t>(*datagram.data())) {
            uint32_t last_index =
              (datagram.data()[1] << 24)
              | (datagram.data()[1 + 1] << 16)
              | (datagram.data()[1 + 2] << 8)
              | (datagram.data()[1 + 3]);
            for(;;){
              auto p = this->output_messages_.begin();
              if (this->output_messages_.end() == p) {
                break;
              }

              if (p->first < last_index) {
                this->output_messages_.erase(p);
                this->last_sent_ = 0;
              }
              else {
                break;
              }
            }

            auto p = this->output_messages_.find(last_index);
            if (this->output_messages_.end() != p) {
              CHECK_EXPECTED_ASYNC(co_await s->write_async(udp_datagram(this->address_, p->second)));
            }

            uint32_t mask =
              (datagram.data()[5] << 24)
              | (datagram.data()[5 + 1] << 16)
              | (datagram.data()[5 + 2] << 8)
              | (datagram.data()[5 + 3]);

            for (int i = 0; 0 != mask && i < 32; ++i) {
              if (1 == (mask & 1)) {
                p = this->output_messages_.find(last_index + i + 1);
                if (this->output_messages_.end() != p) {
                  CHECK_EXPECTED_ASYNC(co_await s->write_async(udp_datagram(this->address_, p->second)));
                }
              }

              mask >>= 1;
            }

            auto mtu =
                (datagram.data()[8 + 1] << 8)
              | (datagram.data()[8 + 2]);
            if (MIN_MTU < mtu) {
              this->mtu_ = mtu;
            }

            co_return expected<void>();
          }

          if (datagram.size() < 33) {
            co_return vds::make_unexpected<std::runtime_error>("Invalid data");
          }

          if (!hmac::verify(
            this->session_key_,
            hash::sha256(),
            datagram.data(), datagram.size() - 32,
            datagram.data() + datagram.size() - 32, 32)) {

            co_return vds::make_unexpected<std::runtime_error>("Invalid signature");
          }

          switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & *datagram.data())) {
          case protocol_message_type_t::SingleData:
          case protocol_message_type_t::RouteSingleData:
          case protocol_message_type_t::ProxySingleData:
          case protocol_message_type_t::Data:
          case protocol_message_type_t::RouteData:
          case protocol_message_type_t::ProxyData: {
            break;
          }
          default: {
            if (datagram.data()[0] != (uint8_t)protocol_message_type_t::ContinueData) {
              co_return vds::make_unexpected<std::runtime_error>("Invalid data");
            }
            break;
          }
          }

           const auto index =
            (datagram.data()[1] << 24)
            | (datagram.data()[1 + 1] << 16)
            | (datagram.data()[1 + 2] << 8)
            | (datagram.data()[1 + 3]);

           std::unique_lock<std::mutex> lock(this->input_mutex_);
           bool is_new = (this->input_messages_.end() == this->input_messages_.find(index));
           if (is_new) {
             this->input_messages_[index] = datagram;
           }
           lock.unlock();

           if(is_new){
             CHECK_EXPECTED_ASYNC(co_await this->continue_process_messages(s));

             if (index > this->last_input_index_ + 32) {
               CHECK_EXPECTED_ASYNC(co_await this->send_acknowledgment(s));
             }
           }
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
          this->last_sent_++;
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

          CHECK_EXPECTED_ASYNC(co_await this->send_acknowledgment(s));

          co_return expected<void>();
        }


      protected:
        const service_provider * sp_;

        std::mutex traffic_mutex_;
        std::map<std::string /*from*/, std::map<std::string /*to*/, std::map<uint8_t /*message_type*/, session_statistic::traffic_info /*size*/>>> traffic_;

        async_task<vds::expected<void>> send_acknowledgment(
          const std::shared_ptr<transport_type>& s) {

          resizable_data_buffer out_message;
          out_message.add((uint8_t)protocol_message_type_t::Acknowledgment);


          std::unique_lock<std::mutex> lock(this->input_mutex_);
          out_message.add((uint8_t)((this->last_input_index_) >> 24));//1
          out_message.add((uint8_t)((this->last_input_index_) >> 16));//1
          out_message.add((uint8_t)((this->last_input_index_) >> 8));//1
          out_message.add((uint8_t)((this->last_input_index_) & 0xFF));//1

          uint32_t mask = 0;
          for (int i = 32; i > 0; --i) {
            mask <<= 1;

            if (this->input_messages_.end() == this->input_messages_.find(this->last_input_index_ + i)) {
              mask |= 1;
            }
          }
          lock.unlock();

          out_message.add((uint8_t)((mask) >> 24));//1
          out_message.add((uint8_t)((mask) >> 16));//1
          out_message.add((uint8_t)((mask) >> 8));//1
          out_message.add((uint8_t)((mask) & 0xFF));//1

          out_message.add((uint8_t)((this->input_mtu_) >> 8));//1
          out_message.add((uint8_t)((this->input_mtu_) & 0xFF));//1

          return s->write_async(udp_datagram(this->address_, out_message.move_data()));
        }

      private:
        static constexpr uint8_t INDEX_SIZE = 4;
        static constexpr uint8_t SIZE_SIZE = 4;

        static constexpr int SEND_TIMEOUT = 600;

        struct send_queue_item_t {
          std::shared_ptr<transport_type> transport;
          uint8_t message_type;
          const_data_buffer target_node;
          const_data_buffer source_node;
          uint16_t hops;
          const_data_buffer message;
        };
        bool failed_state_;
        int check_mtu_;
        int last_sent_;
        network_address address_;
        const_data_buffer this_node_id_;
        const_data_buffer partner_node_id_;
        const_data_buffer session_key_;

        uint16_t mtu_;
        uint16_t input_mtu_;
        
        std::mutex output_mutex_;
        uint32_t last_output_index_;
        std::map<uint32_t, const_data_buffer> output_messages_;

        std::mutex input_mutex_;
        uint32_t last_input_index_;
        std::map<uint32_t, const_data_buffer> input_messages_;

        vds::async_task<vds::expected<void>> send_message_async(
          const std::shared_ptr<transport_type>& s,
          uint8_t message_type,
          const const_data_buffer target_node,
          const const_data_buffer source_node,
          const uint8_t hops,
          const const_data_buffer message) {

          GET_EXPECTED_ASYNC(indexes, this->prepare_to_send(
            message_type,
            target_node,
            source_node,
            hops,
            message));

          for (uint32_t start_index = std::get<0>(indexes); start_index < std::get<1>(indexes); ++start_index) {
            CHECK_EXPECTED_ASYNC(co_await s->write_async(udp_datagram(this->address_, this->output_messages_[start_index])));
          }
          co_return expected<void>();
        }

        vds::expected<std::tuple<uint32_t, uint32_t>> prepare_to_send(
          uint8_t message_type,
          const const_data_buffer target_node,
          const const_data_buffer source_node,
          const uint8_t hops,
          const const_data_buffer message) {

          std::unique_lock<std::mutex> lock(this->output_mutex_);
          uint32_t start_index = this->last_output_index_;

          size_t total_size = 1 + 4 + 32 + message.size();
          if (this->this_node_id_ == source_node) {
            if (this->partner_node_id_ != target_node) {
              total_size += 32;
            }
          }
          else {
            total_size += 32 + 32 + 1;
          }

          resizable_data_buffer buffer;

          if (total_size < this->mtu_) {
            if (this->this_node_id_ == source_node) {
              if (this->partner_node_id_ == target_node) {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::SingleData | message_type));
                buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
                buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
              }
              else {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteSingleData | message_type));
                buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
                buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
                buffer += target_node;
              }
            }
            else {
              buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxySingleData | message_type));
              buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
              buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
              buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
              buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
              buffer += target_node;
              buffer += source_node;
              buffer.add(hops);
            }

            buffer += message;
            GET_EXPECTED(sig, hmac::signature(
              this->session_key_,
              hash::sha256(),
              buffer.data(),
              buffer.size()));
            buffer += sig;

            const_data_buffer datagram = buffer.move_data();
            vds_assert(datagram.size() <= this->mtu_);

            this->output_messages_[this->last_output_index_] = datagram;
            this->last_output_index_++;
          }
          else {
            uint32_t offset;

            if (this->this_node_id_ == source_node) {
              if (this->partner_node_id_ == target_node) {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::Data | message_type));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
                buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
                buffer.add((uint8_t)((message.size()) >> 24));//1
                buffer.add((uint8_t)((message.size()) >> 16));//1
                buffer.add((uint8_t)((message.size()) >> 8));//1
                buffer.add((uint8_t)((message.size()) & 0xFF));//1

                buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32));
                offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32);
              }
              else {
                buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteData | message_type));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
                buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
                buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
                buffer.add((uint8_t)((message.size()) >> 24));//1
                buffer.add((uint8_t)((message.size()) >> 16));//1
                buffer.add((uint8_t)((message.size()) >> 8));//1
                buffer.add((uint8_t)((message.size()) & 0xFF));//1
                vds_assert(target_node.size() == 32);
                buffer += target_node;//32


                buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32));
                offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32);
              }
            }
            else {
              buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxyData | message_type));//1
              buffer.add((uint8_t)((this->last_output_index_) >> 24));//1
              buffer.add((uint8_t)((this->last_output_index_) >> 16));//1
              buffer.add((uint8_t)((this->last_output_index_) >> 8));//1
              buffer.add((uint8_t)((this->last_output_index_) & 0xFF));//1
              buffer.add((uint8_t)((message.size()) >> 24));//1
              buffer.add((uint8_t)((message.size()) >> 16));//1
              buffer.add((uint8_t)((message.size()) >> 8));//1
              buffer.add((uint8_t)((message.size()) & 0xFF));//1
              vds_assert(target_node.size() == 32);
              vds_assert(source_node.size() == 32);
              buffer += target_node;//32
              buffer += source_node;//32
              buffer.add((uint8_t)(hops));//1

              buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32));
              offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32);
            }

            GET_EXPECTED(sig, hmac::signature(
              this->session_key_,
              hash::sha256(),
              buffer.data(),
              buffer.size()));

            buffer += sig;
            const_data_buffer datagram = buffer.move_data();
            vds_assert(datagram.size() <= this->mtu_);

            this->output_messages_[this->last_output_index_] = datagram;
            this->last_output_index_++;

            for (;;) {
              auto size = this->mtu_ - (1 + 4 + 32);
              if (size > message.size() - offset) {
                size = message.size() - offset;
              }

              resizable_data_buffer buffer;
              buffer.add((uint8_t)protocol_message_type_t::ContinueData);//1
              buffer.add(this->last_output_index_ >> 24);//1
              buffer.add(this->last_output_index_ >> 16);//1
              buffer.add(this->last_output_index_ >> 8);//1
              buffer.add(this->last_output_index_);//1
              buffer.add(message.data() + offset, size);//

              GET_EXPECTED(sig, hmac::signature(
                this->session_key_,
                hash::sha256(),
                buffer.data(),
                buffer.size()));
              buffer += sig;

              const_data_buffer datagram = buffer.move_data();

              vds_assert(datagram.size() <= this->mtu_);
              this->output_messages_[this->last_output_index_] = datagram;
              this->last_output_index_++;

              if (offset + size >= message.size()) {
                break;
              }
              offset += size;
            }
          }

          vds_assert(start_index < this->last_output_index_);

          if (this->last_output_index_ < start_index) {
            this->failed_state_ = true;
            return make_unexpected<std::runtime_error>("Overflow pipeline");
          }
          return std::make_tuple(start_index, this->last_output_index_);
        }

        vds::async_task<vds::expected<void>> continue_process_messages(
          const std::shared_ptr<transport_type>& s) {

          for (;;) {
            std::unique_lock<std::mutex> lock(this->input_mutex_);

            auto p = this->input_messages_.begin();
            if (p == this->input_messages_.end() || p->first > this->last_input_index_) {
              co_return expected<void>();
            }

            if(p->first < this->last_input_index_) {
              this->input_messages_.erase(p);
              continue;
            }

            //auto p = this->input_messages_.find(this->last_input_index_);
            //if (p == this->input_messages_.end()) {
            //  co_return expected<void>();
            //}


            switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & *p->second.data())) {
            case protocol_message_type_t::SingleData:
            case protocol_message_type_t::RouteSingleData:
            case protocol_message_type_t::ProxySingleData: {
              auto message_type = (uint8_t)(p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand);

              const_data_buffer target_node;
              const_data_buffer source_node;
              uint16_t hops;
              uint32_t index;
              const_data_buffer message;

              switch (
                static_cast<protocol_message_type_t>(
                (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
                ) {
              case protocol_message_type_t::SingleData: {
                target_node = this->this_node_id_;
                source_node = this->partner_node_id_;
                hops = 0;

                message = const_data_buffer(p->second.data() + 1 + 4, p->second.size() - 1 - 4 - 32);
                break;
              }

              case protocol_message_type_t::RouteSingleData: {
                target_node = const_data_buffer(p->second.data() + 1 + 4, 32);
                source_node = this->partner_node_id_;
                hops = 0;
                message = const_data_buffer(p->second.data() + 1 + 4 + 32, p->second.size() - 1 - 4 - 32 - 32);
                break;
              }

              case protocol_message_type_t::ProxySingleData: {
                target_node = const_data_buffer(p->second.data() + 1 + 4, 32);
                source_node = const_data_buffer(p->second.data() + 1 + 4 + 32, 32);
                hops = p->second.data()[1 + 4 + 32 + 32];

                message = const_data_buffer(p->second.data() + 1 + 4 + 32 + 32 + 1, p->second.size() - (1 + 4 + 32 + 32 + 1 + 32));
                break;
              default:
                vds_assert(false);
                break;
              }
              }

              const auto message_size = message.size();
              this->last_input_index_++;
              lock.unlock();

              GET_EXPECTED_ASYNC(is_good, co_await static_cast<implementation_class *>(this)->process_message(
                s,
                message_type,
                target_node,
                source_node,
                hops,
                message));

              std::unique_lock<std::mutex> traffic_lock(this->traffic_mutex_);
              if (is_good) {
                this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_count_++;
                this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_traffic_ += message_size;
              }
              else {
                this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_count_++;
                this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_traffic_ += message_size;
              }
              traffic_lock.unlock();

              break;

            }

            case protocol_message_type_t::Data:
            case protocol_message_type_t::RouteData:
            case protocol_message_type_t::ProxyData: {
              auto message_type = p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand;
              size_t size =
                uint32_t(p->second.data()[1 + 4] << 24)
                | uint32_t(p->second.data()[1 + 4 + 1] << 16)
                | uint32_t(p->second.data()[1 + 4 + 2] << 8)
                | uint32_t(p->second.data()[1 + 4 + 3]);

              const_data_buffer target_node;
              const_data_buffer source_node;
              uint16_t hops;
              resizable_data_buffer message;

              switch (
                static_cast<protocol_message_type_t>(
                (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
                ) {
              case protocol_message_type_t::Data: {
                if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32)) {
                  co_return vds::make_unexpected<std::runtime_error>("Invalid data");
                }
                vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32));
                size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32);

                target_node = this->this_node_id_;
                source_node = this->partner_node_id_;
                hops = 0;
                message.add(p->second.data() + 1 + 4 + SIZE_SIZE, p->second.size() - (1 + 4 + SIZE_SIZE + 32));
                break;
              }

              case protocol_message_type_t::RouteData: {
                if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32)) {
                  co_return vds::make_unexpected<std::runtime_error>("Invalid data");
                }
                vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32 + 32));
                size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32);

                target_node = const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE, 32);
                source_node = this->partner_node_id_;
                hops = 0;
                message.add(p->second.data() + (1 + 4 + SIZE_SIZE + 32), p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32));
                break;
              }

              case protocol_message_type_t::ProxyData: {
                if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32)) {
                  co_return vds::make_unexpected<std::runtime_error>("Invalid data");
                }
                vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32));
                size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32);

                target_node = const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE, 32);
                source_node = const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE + 32, 32);

                hops = p->second.data()[1 + 4 + SIZE_SIZE + 32 + 32];
                message.add(p->second.data() + (1 + 4 + SIZE_SIZE + 32 + 32 + 1), p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32));
                break;
              }
              default:
                co_return vds::make_unexpected<std::runtime_error>("Invalid program");
              }

              uint32_t index = 0;
              for (;;) {
                auto p1 = this->input_messages_.find(this->last_input_index_ + ++index);
                if (this->input_messages_.end() == p1) {
                  co_return expected<void>();
                }

                if ((uint8_t)protocol_message_type_t::ContinueData != p1->second.data()[0]) {
                  co_return vds::make_unexpected<std::runtime_error>("Invalid data");
                }

                if (size < p1->second.size() - (1 + 4 + 32)) {
                  co_return vds::make_unexpected<std::runtime_error>("Invalid data");
                }

                message.add(p1->second.data() + (1 + 4), p1->second.size() - (1 + 4 + 32));
                vds_assert(p1->second.size() > (1 + 4 + 32));
                size -= p1->second.size() - (1 + 4 + 32);

                if (0 == size) {
                  const auto message_size = message.size();
                  this->last_input_index_ += index + 1;
                  lock.unlock();

                  GET_EXPECTED_ASYNC(is_good, co_await static_cast<implementation_class *>(this)->process_message(
                    s,
                    message_type,
                    target_node,
                    source_node,
                    hops,
                    message.move_data()));

                  std::unique_lock<std::mutex> traffic_lock(this->traffic_mutex_);
                  if (is_good) {
                    this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_count_++;
                    this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_traffic_ += message_size;
                  }
                  else {
                    this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_count_++;
                    this->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_traffic_ += message_size;
                  }
                  traffic_lock.unlock();

                  break;
                }
              }

              break;
            }
            default: {
              //auto p1 = this->input_messages_.find(this->last_input_index_ - 4);
              //auto p2 = this->input_messages_.find(this->last_input_index_ - 3);
              //auto p3 = this->input_messages_.find(this->last_input_index_ - 2);
              //auto p4 = this->input_messages_.find(this->last_input_index_ - 1);
              vds_assert(false);
              break;
            }
            }
          }
          co_return expected<void>();
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
