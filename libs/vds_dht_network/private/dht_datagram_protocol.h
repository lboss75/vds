#ifndef __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
#define __VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include "udp_socket.h"
#include "async_task.h"
#include "const_data_buffer.h"
#include "udp_datagram_size_exception.h"
#include "vds_debug.h"
#include "dht_message_type.h"

namespace vds {
  namespace dht {
    namespace network {

      template <typename implementation_class>
      class dht_datagram_protocol : public std::enable_shared_from_this<implementation_class> {
      public:
        dht_datagram_protocol(
            const network_address & address,
            const const_data_buffer & this_node_id)
        : address_(address),
          this_node_id_(this_node_id),
          output_sequence_number_(0),
          mtu_(65507),
          input_sequence_number_(0),
          next_sequence_number_(0),
          next_process_index_(0)
        {
        }

        async_task<> send_message(
            const service_provider &sp,
            const udp_socket & s,
            uint8_t message_type,
            const const_data_buffer & message) {
          vds_assert(message.size() < 0xFFFF);

          return [pthis = this->shared_from_this(), sp, s, message_type, message](
              const async_result<> &result) {
            resizable_data_buffer buffer;

            if (message.size() < pthis->mtu_ - 5) {
              buffer.add((uint8_t) ((uint8_t)message_type_t::SingleData | message_type));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 24));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 16));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 8));
              buffer.add((uint8_t) (pthis->output_sequence_number_));
              buffer += message;

              const_data_buffer datagram(buffer.data(), buffer.size());
              s.write_async(udp_datagram(pthis->address_, datagram))
                  .execute([pthis, sp, s, message_type, message, result, datagram](
                      const std::shared_ptr<std::exception> &ex) {
                    if (ex) {
                      auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                      if (datagram_error) {
                        pthis->mtu_ /= 2;
                        pthis->send_message(sp, s, message_type, message)
                            .execute([result](const std::shared_ptr<std::exception> &ex) {
                              if (ex) {
                                result.error(ex);
                              } else {
                                result.done();
                              }
                            });
                      } else {
                        result.error(ex);
                      }
                    } else {
                      pthis->output_messages_[pthis->output_sequence_number_] = datagram;
                      pthis->output_sequence_number_++;
                      result.done();
                    }
                  });
            } else {
              buffer.add((uint8_t) ((uint8_t)message_type_t::Data | message_type));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 24));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 16));
              buffer.add((uint8_t) (pthis->output_sequence_number_ >> 8));
              buffer.add((uint8_t) (pthis->output_sequence_number_));
              buffer.add((uint8_t) (message.size() >> 8));
              buffer.add((uint8_t) (message.size() & 0xFF));
              buffer.add(message.data(), pthis->mtu_ - 7);

              auto offset = pthis->mtu_ - 7;

              const_data_buffer datagram(buffer.data(), buffer.size());
              s.write_async(udp_datagram(pthis->address_, datagram))
                  .execute([pthis, sp, s, message_type, message, offset, result, datagram](
                      const std::shared_ptr<std::exception> &ex) {
                    if (ex) {
                      auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                      if (datagram_error) {
                        pthis->mtu_ /= 2;
                        pthis->send_message(sp, s, message_type, message)
                            .execute([result](const std::shared_ptr<std::exception> &ex) {
                              if (ex) {
                                result.error(ex);
                              } else {
                                result.done();
                              }
                            });
                      } else {
                        result.error(ex);
                      }
                    } else {
                      pthis->output_messages_[pthis->output_sequence_number_] = datagram;
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

        async_task<> process_datagram(
            const service_provider & sp,
            const udp_socket & s,
            const const_data_buffer & datagram){
          if(datagram.size() == 0){
            return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
          }

          switch ((message_type_t)*datagram.data()){
            case message_type_t::Acknowledgment: {
              if (datagram.size() < 9) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }

              size_t index = (datagram.data()[1] << 24)
                             | (datagram.data()[2] << 16)
                             | (datagram.data()[3] << 8)
                             | (datagram.data()[4]);

              size_t mask = (datagram.data()[5] << 24)
                            | (datagram.data()[6] << 16)
                            | (datagram.data()[7] << 8)
                            | (datagram.data()[8]);

              while(!this->input_messages_.empty()){
                auto first_index = this->input_messages_.begin()->first;
                if(first_index < index){
                  this->input_messages_.erase(this->input_messages_.begin());
                }
                else {
                  break;
                }
              }

              return this->repeat_message(sp, s, mask, index);
            }
            default: {
              if (datagram.size() < 5) {
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }

              size_t index = (datagram.data()[1] << 24)
                | (datagram.data()[2] << 16)
                | (datagram.data()[3] << 8)
                | (datagram.data()[4]);

              if(this->input_messages_.end() == this->input_messages_.find(index)){
                this->input_messages_[index] = datagram;

                if(index == this->next_sequence_number_){
                  do{
                    this->next_sequence_number_++;
                  } while(this->input_messages_.end() != this->input_messages_.find(this->next_sequence_number_));

                  return this->continue_process_messages(sp, s);
                }
              }

              return async_task<>::empty();
            }
          }
        }


      private:
        network_address address_;
        const_data_buffer this_node_id_;

        size_t output_sequence_number_;
        std::map<size_t, const_data_buffer> output_messages_;
        size_t mtu_;

        size_t input_sequence_number_;
        std::map<size_t, const_data_buffer> input_messages_;

        size_t next_sequence_number_;
        size_t next_process_index_;

        void continue_send_message(
            const service_provider &sp,
            const udp_socket & s,
            const const_data_buffer & message,
            size_t offset,
            const async_result<> &result){

          auto size = this->mtu_ - 5;
          if(size > message.size() - offset){
            size = message.size() - offset;
          }

          resizable_data_buffer buffer;
          buffer.add((uint8_t) message_type_t::ContinueData);
          buffer.add((uint8_t) (this->output_sequence_number_ >> 24));
          buffer.add((uint8_t) (this->output_sequence_number_ >> 16));
          buffer.add((uint8_t) (this->output_sequence_number_ >> 8));
          buffer.add((uint8_t) (this->output_sequence_number_));
          buffer.add(message.data() + offset, size);

          const_data_buffer datagram(buffer.data(), buffer.size());
          s.write_async(udp_datagram(this->address_, datagram))
              .execute(
                  [pthis = this->shared_from_this(), sp, s, message, offset, size, result, datagram](
                  const std::shared_ptr<std::exception> &ex) {
                if (ex) {
                  auto datagram_error = std::dynamic_pointer_cast<udp_datagram_size_exception>(ex);
                  if (datagram_error) {
                    pthis->mtu_ /= 2;
                    pthis->continue_message(sp, s, message, offset, result);
                  } else {
                    result.error(ex);
                  }
                } else {
                  pthis->output_messages_[pthis->output_sequence_number_] = datagram;
                  pthis->output_sequence_number_++;
                  offset += size;
                  if(offset < message.size()) {
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
            const udp_socket & s) {
          auto p = this->input_messages_.find(this->next_process_index_);
          if (this->input_messages_.end() == p) {
            return async_task<>::empty();
          }

          switch ((message_type_t)((uint8_t)message_type_t::SpecialCommand & p->second.data()[0])) {
            case message_type_t::SingleData: {
              auto message_type = (uint8_t)(p->second.data()[0] & ~(uint8_t) message_type_t::SpecialCommand);
              const_data_buffer message(p->second.data() + 5, p->second.size() - 5);

              this->input_messages_.erase(this->next_process_index_);
              this->next_process_index_++;

              return static_cast<implementation_class *>(this)->process_message(
                  sp,
                  message_type,
                  message).then([sp, s, pthis = this->shared_from_this()]() {
                pthis->continue_process_messages(sp, s);
              });
            }
            case message_type_t::Data: {
              auto message_type = p->second.data()[0] & ~(uint8_t)message_type_t::SpecialCommand;
              size_t size = (p->second.data()[5] << 8) | (p->second.data()[6]);

              if(size <= p->second.size() - 7){
                return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
              }
              size -= p->second.size() - 7;

              resizable_data_buffer message;
              message.add(p->second.data() + 7, p->second.size() - 7);

              for(auto index = this->next_process_index_ + 1;; ++index){
                auto p = this->input_messages_.find(index);
                if (this->input_messages_.end() == p) {
                  return async_task<>::empty();
                }

                if((uint8_t)message_type_t::ContinueData != p->second.data()[0]){
                  return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
                }

                if(size < p->second.size() - 1){
                  return async_task<>(std::make_shared<std::runtime_error>("Invalid data"));
                }

                message.add(p->second.data() + 1, p->second.size() - 1);
                size -= p->second.size() - 1;

                if(0 == size){
                  while(index <= this->next_process_index_) {
                    this->input_messages_.erase(this->next_process_index_);
                    this->next_process_index_++;
                  }

                  return static_cast<implementation_class *>(this)->process_message(
                      sp,
                      message_type,
                      const_data_buffer(message.data(), message.size())).then([sp, s, pthis = this->shared_from_this()]() {
                    pthis->continue_process_messages(sp, s);
                  });
                }
              }

              break;
            }
            default:
              throw std::runtime_error("Unexcpected");
          }

          throw std::runtime_error("Unexcpected");
        }

        async_task<> repeat_message(
            const service_provider & sp,
            const udp_socket & s,
            size_t mask,
            size_t index){

          while(mask > 0){
            if(1 == (mask & 1)){
              auto p = this->input_messages_.find(index);
              if(this->input_messages_.end() != p){

                mask >>= 1;
                ++index;

                return s.write_async(udp_datagram(this->address_, p->second))
                    .then([pthis = this->shared_from_this(), sp, s, mask, index](){
                      return pthis->repeat_message(sp, s, mask, index);
                    });
              }
            }

            mask >>= 1;
            ++index;
          }

          return async_task<>::empty();
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_DATAGRAM_PROTOCOL_H_
