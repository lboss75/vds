#ifndef __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
#define __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include <udp_socket.h>
#include "async_task.h"
#include "const_data_buffer.h"
#include "udp_datagram_size_exception.h"
#include "vds_debug.h"
#include "messages/dht_message_type.h"
#include "network_address.h"
#include "debug_mutex.h"
#include <queue>
#include "vds_exceptions.h"

namespace vds {
  namespace dht {
    namespace network {

      template <typename implementation_class, typename transport_type>
      class dht_datagram_protocol : public std::enable_shared_from_this<implementation_class> {
      public:
        dht_datagram_protocol(
            const network_address & address,
            const const_data_buffer & this_node_id,
            uint32_t session_id)
        : address_(address),
          this_node_id_(this_node_id),
          session_id_(session_id),
          output_sequence_number_(0),
          mtu_(65507),
          next_sequence_number_(0),
          skipped_datagrams_(0)
        {
        }

        void send_message(
          const service_provider &sp,
          const std::shared_ptr<transport_type> & s,
          uint8_t message_type,
          const const_data_buffer & target_node,
          const const_data_buffer & source_node,
          const uint16_t hops,
          const const_data_buffer & message) {
          vds_assert(message.size() < 0xFFFF);
          vds_assert(target_node != this->this_node_id_);

          std::unique_lock<std::mutex> lock(this->send_mutex_);

          this->message_count_[message_type]++;
          this->message_size_[message_type] += message.size();

          for(const auto & p : this->send_queue_){
            if(p.message_type == message_type
              && p.target_node == target_node
              && p.source_node == source_node
              && p.message == message ) {
              return;
            }
          }

          sp.get<logger>()->trace(
            "dht_session",
            sp,
            "send %d from %s to %s",
            message_type,
            base64::from_bytes(source_node).c_str(),
            base64::from_bytes(target_node).c_str());

          const bool need_start = this->send_queue_.empty();
          this->send_queue_.push_back(send_queue_item_t {
            sp,
            s,
            message_type,
            target_node,
            source_node,
            hops, message });

          lock.unlock();

          if (need_start) {
            this->continue_send(false);
          }
        }

        async_task<> process_datagram(
            const service_provider & scope,
            const std::shared_ptr<transport_type> & s,
            const const_data_buffer & datagram){
          auto sp = scope.create_scope(__FUNCTION__);
          std::unique_lock<std::mutex> lock(this->input_mutex_);
          if(datagram.size() == 0){
            return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
          }

          switch (static_cast<protocol_message_type_t>(*datagram.data())){
            case protocol_message_type_t::Acknowledgment: {
              if (datagram.size() < 5) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }
              const uint32_t session_id = (datagram.data()[1] << 24)
                | (datagram.data()[2] << 16)
                | (datagram.data()[3] << 8)
                | (datagram.data()[4]);

              if(this->session_id_ != session_id) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid session_id"));
              }

              logger::get(sp)->trace(
                "DHT",
                sp,
                "Acknowledgment from %s",
                this->address_.to_string().c_str());


              return async_task<>::empty();
            }
            default: {
              if (datagram.size() < 5) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }

              const uint32_t index = (datagram.data()[1] << 24)
                | (datagram.data()[2] << 16)
                | (datagram.data()[3] << 8)
                | (datagram.data()[4]);

              if(index >= this->next_sequence_number_ && this->input_messages_.end() == this->input_messages_.find(index)){
                this->input_messages_[index] = datagram;
                logger::get(sp)->trace(
                  "DHT",
                  sp,
                  "Input Datagram[%d]: %d bytes from %s",
                  index,
                  datagram.size(),
                  this->address_.to_string().c_str());

                return this->continue_process_messages(sp, s, lock);
              }

              return async_task<>::empty();
            }
          }
        }

        const network_address & address() const {
          return this->address_;
        }

        async_task<> on_timer(const service_provider & sp, const std::shared_ptr<transport_type> & s) {

          std::shared_lock<std::shared_mutex> lock(this->output_mutex_);
          std::unique_lock<std::mutex> input_lock(this->input_mutex_);

          uint8_t buffer[13];

          buffer[0] = static_cast<uint8_t>(protocol_message_type_t::Acknowledgment);
          buffer[1] = static_cast<uint8_t>(this->session_id_ >> 24);
          buffer[2] = static_cast<uint8_t>(this->session_id_ >> 16);
          buffer[3] = static_cast<uint8_t>(this->session_id_ >> 8);
          buffer[4] = static_cast<uint8_t>(this->session_id_);

          sp.get<logger>()->trace("DHT", sp, "Send Acknowledgment %d", this->next_sequence_number_);
          return s->write_async(sp, udp_datagram(this->address_, const_data_buffer(buffer, sizeof(buffer)), false));
        }

        const const_data_buffer this_node_id() const {
          return this->this_node_id_;
        }

      private:
        network_address address_;
        const_data_buffer this_node_id_;
        uint32_t session_id_;

        std::shared_mutex output_mutex_;
        uint32_t output_sequence_number_;
        uint32_t mtu_;

        std::mutex input_mutex_;
        std::map<uint32_t, const_data_buffer> input_messages_;
        uint32_t next_sequence_number_;

        mutable std::mutex send_mutex_;
        struct send_queue_item_t {
          service_provider sp;
          std::shared_ptr<transport_type> transport;
          uint8_t message_type;
          const_data_buffer target_node;
          const_data_buffer source_node;
          uint16_t hops;
          const_data_buffer message;
        };
        std::list<send_queue_item_t> send_queue_;

        std::map<uint8_t, uint64_t> message_count_;
        std::map<uint8_t, uint64_t> message_size_;
        uint64_t skipped_datagrams_;

        async_task<> send_message_async(
          const service_provider &sp,
          const std::shared_ptr<transport_type> & s,
          uint8_t message_type,
          const const_data_buffer & target_node,
          const const_data_buffer & source_node,
          const uint16_t hops,
          const const_data_buffer & message) {
          vds_assert(message.size() < 0xFFFF);

          return [
              pthis = this->shared_from_this(),
                  sp,
              s,
              message_type,
              target_node,
              source_node,
              hops,
              message](
            const async_result<> &result) {
            std::unique_lock<std::shared_mutex> lock(pthis->output_mutex_);
            resizable_data_buffer buffer;

            if (message.size() < pthis->mtu_ - 5) {
              auto expected_index = pthis->output_sequence_number_;
              lock.unlock();

              buffer.add((uint8_t)((uint8_t)protocol_message_type_t::SingleData | message_type));
              buffer.add((uint8_t)(expected_index >> 24));
              buffer.add((uint8_t)(expected_index >> 16));
              buffer.add((uint8_t)(expected_index >> 8));
              buffer.add((uint8_t)(expected_index));
              buffer += target_node;
              buffer += source_node;
              buffer.add((uint8_t)(hops >> 8));
              buffer.add((uint8_t)(hops));
              buffer += message;

              const_data_buffer datagram = buffer.move_data();
              s->write_async(sp, udp_datagram(pthis->address_, datagram, false))
                .execute([
                    pthis,
                             sp,
                             s,
                             message_type,
                             target_node,
                             source_node,
                             hops,
                             message,
                             result,
                             datagram,
                             expected_index](
                  const std::shared_ptr<std::exception> &ex) {
                std::unique_lock<std::shared_mutex> lock(pthis->output_mutex_);
                if (ex) {
                  auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                  if (datagram_error) {
                    pthis->mtu_ /= 2;
                    pthis->send_message_async(
                        sp,
                        s,
                        message_type,
                        target_node,
                        source_node,
                        hops,
                        message)
                      .execute([result](const std::shared_ptr<std::exception> &ex) {
                      if (ex) {
                        result.error(ex);
                      }
                      else {
                        result.done();
                      }
                    });
                    return;
                  }
                  else {
                    auto sys_error = std::dynamic_pointer_cast<std::system_error>(ex);
                    if (sys_error) {
                      if(
                          std::system_category() != sys_error->code().category()
                        || EPIPE != sys_error->code().value()){
                        result.error(ex);
                        return;
                      }
                    }
                    else {
                      result.error(ex);
                      return;
                    }
                  }
                }

                vds_assert(expected_index == pthis->output_sequence_number_);
                logger::get(sp)->trace(
                  "DHT",
                  sp,
                  "Out datagram[%d]: %d bytes to %s",
                  pthis->output_sequence_number_,
                  datagram.size(),
                  pthis->address_.to_string().c_str());
                pthis->output_sequence_number_++;
                result.done();
              });
            }
            else {
              buffer.add((uint8_t)((uint8_t)protocol_message_type_t::Data | message_type));
              buffer.add((uint8_t)(pthis->output_sequence_number_ >> 24));
              buffer.add((uint8_t)(pthis->output_sequence_number_ >> 16));
              buffer.add((uint8_t)(pthis->output_sequence_number_ >> 8));
              buffer.add((uint8_t)(pthis->output_sequence_number_));
              buffer.add((uint8_t)((message.size() + target_node.size() + source_node.size()) >> 8));
              buffer.add((uint8_t)((message.size() + target_node.size() + source_node.size()) & 0xFF));
              vds_assert(target_node.size() == 32);
              buffer += target_node;
              buffer += source_node;
              buffer.add((uint8_t)(hops >> 8));
              buffer.add((uint8_t)(hops));
              buffer.add(message.data(), pthis->mtu_ - 57);

              auto offset = pthis->mtu_ - 57;

              const_data_buffer datagram = buffer.move_data();
              s->write_async(sp, udp_datagram(pthis->address_, datagram))
                .execute([
                    pthis,
                             sp,
                             s,
                             message_type,
                             target_node,
                             source_node,
                             hops,
                             message,
                             offset,
                             result,
                             datagram,
                             expected_index = pthis->output_sequence_number_](
                  const std::shared_ptr<std::exception> &ex) {
                std::unique_lock<std::shared_mutex> lock(pthis->output_mutex_);
                if (ex) {
                  auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                  if (datagram_error) {
                    pthis->mtu_ /= 2;
                    pthis->send_message_async(
                        sp,
                        s,
                        message_type,
                        target_node,
                        source_node,
                        hops,
                        message)
                      .execute([result](const std::shared_ptr<std::exception> &ex) {
                      if (ex) {
                        result.error(ex);
                      }
                      else {
                        result.done();
                      }
                    });
                  }
                  else {
                    result.error(ex);
                  }
                }
                else {
                  logger::get(sp)->trace(
                    "DHT",
                    sp,
                    "Out datagram[%d]: %d bytes to %s",
                    pthis->output_sequence_number_,
                    datagram.size(),
                    pthis->address_.to_string().c_str());
                  vds_assert(expected_index == pthis->output_sequence_number_);
                  pthis->output_sequence_number_++;
                  pthis->continue_send_message(
                    sp,
                    s,
                    message,
                    offset,
                    result);
                }
              });
            }
          };
        }

        void continue_send_message(
            const service_provider &sp,
            const std::shared_ptr<transport_type> & s,
            const const_data_buffer & message,
            size_t offset,
            const async_result<> &result){

          auto size = this->mtu_ - 5;
          if(size > message.size() - offset){
            size = message.size() - offset;
          }

          resizable_data_buffer buffer;
          buffer.add((uint8_t) protocol_message_type_t::ContinueData);
          buffer.add((uint8_t) (this->output_sequence_number_ >> 24));
          buffer.add((uint8_t) (this->output_sequence_number_ >> 16));
          buffer.add((uint8_t) (this->output_sequence_number_ >> 8));
          buffer.add((uint8_t) (this->output_sequence_number_));
          buffer.add(message.data() + offset, size);

          const_data_buffer datagram = buffer.move_data();
          s->write_async(sp, udp_datagram(this->address_, datagram))
              .execute(
                  [pthis = this->shared_from_this(), sp, s, message, offset, size, result, datagram](
                  const std::shared_ptr<std::exception> &ex) {
                std::unique_lock<std::shared_mutex> lock(pthis->output_mutex_);
                if (ex) {
                  auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                  if (datagram_error) {
                    pthis->mtu_ /= 2;
                    pthis->continue_send_message(sp, s, message, offset, result);
                  } else {
                    result.error(ex);
                  }
                } else {
                  logger::get(sp)->trace("DHT", sp, "Out datagram[%d]: %d bytes", pthis->output_sequence_number_, datagram.size());
                  ++(pthis->output_sequence_number_);
                 if(offset + size < message.size()) {
                    pthis->continue_send_message(
                        sp,
                        s,
                        message,
                        offset + size,
                        result);
                  }
                  else {
                    result.done();
                  }
                }
              });
        }

        async_task<> continue_process_messages(
            const service_provider &sp,
            const std::shared_ptr<transport_type> & s,
            std::unique_lock<std::mutex> & locker
        ) {
          
          for (auto p = this->input_messages_.begin(); p != this->input_messages_.end(); ++p) {
            switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])) {
            case protocol_message_type_t::SingleData: {
              auto message_type = (uint8_t)(p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand);
              const_data_buffer message(p->second.data() + 5, p->second.size() - 5);

              logger::get(sp)->trace("DHT", sp, "Processed datagram %d", p->first);
              for(auto premove = this->input_messages_.begin(); premove != p; ++premove) {
                ++this->skipped_datagrams_;
              }
              this->input_messages_.erase(this->input_messages_.begin(), p);
              this->input_messages_.erase(p);

              locker.unlock();
              return static_cast<implementation_class *>(this)->process_message(
                sp,
                s,
                message_type,
                message).then([sp, s, pthis = this->shared_from_this()]() {
                std::unique_lock<std::mutex> lock(pthis->input_mutex_);
                return pthis->continue_process_messages(sp, s, lock);
              });
            }
            case protocol_message_type_t::Data: {
              auto message_type = p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand;
              size_t size = (p->second.data()[5] << 8) | (p->second.data()[6]);

              if (size <= p->second.size() - 7) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }
              size -= p->second.size() - 7;

              resizable_data_buffer message;
              message.add(p->second.data() + 7, p->second.size() - 7);

              for (auto index = p->first + 1;; ++index) {
                auto p1 = this->input_messages_.find(index);
                if (this->input_messages_.end() == p1) {
                  break;
                }

                if ((uint8_t)protocol_message_type_t::ContinueData != p1->second.data()[0]) {
                  return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
                }

                if (size < p1->second.size() - 1) {
                  return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
                }

                message.add(p1->second.data() + 1, p1->second.size() - 1);
                size -= p1->second.size() - 1;

                if (0 == size) {
                  for (auto premove = this->input_messages_.begin(); premove != p; ++premove) {
                    ++this->skipped_datagrams_;
                  }
                  this->input_messages_.erase(this->input_messages_.begin(), p1);
                  this->input_messages_.erase(p1);

                  locker.unlock();
                  return static_cast<implementation_class *>(this)->process_message(
                    sp,
                    s,
                    message_type,
                    message.move_data()).then([sp, s, pthis = this->shared_from_this()]() {
                    std::unique_lock<std::mutex> lock(pthis->input_mutex_);
                    pthis->continue_process_messages(sp, s, lock);
                  });
                }
              }

              break;
            }
            }
          }
          return async_task<>::empty();
        }

        void continue_send(bool remove_first) {
          std::unique_lock<std::mutex> lock(this->send_mutex_);

          if (remove_first) {
            this->send_queue_.pop_front();
          }

          if (this->send_queue_.empty()) {
            return;
          }

          const auto & p = this->send_queue_.front();
          auto sp = p.sp;
          auto s = p.transport;
          auto message_type = p.message_type;
          auto target_node = p.target_node;
          auto source_node = p.source_node;
          auto hops = p.hops;
          auto message = p.message;
          lock.unlock();

          this->send_message_async(
            sp,
            s,
            message_type,
            target_node,
            source_node,
            hops,
            message).execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception> & ex) {
            if(ex) {
              logger::get(sp)->warning("DHT", sp, "%s at send dht datagram", ex->what());
            }
            mt_service::async(sp, [pthis]() {
              pthis->continue_send(true);
            });
          });
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
