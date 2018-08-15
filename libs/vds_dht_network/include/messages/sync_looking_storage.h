#ifndef __VDS_DHT_NETWORK_SYNC_LOOKING_STORAGE_H_
#define __VDS_DHT_NETWORK_SYNC_LOOKING_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "messages/sync_base_message.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_looking_storage_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_looking_storage_request;

        sync_looking_storage_request(
          const const_data_buffer &object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          uint32_t object_size)
        : sync_base_message_request(
          object_id,
          generation,
          current_term,
          commit_index,
          last_applied),
          object_size_(object_size){
        }

        sync_looking_storage_request(
            binary_deserializer & s)
        : sync_base_message_request(s){
          s
            >> this->object_size_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
            << this->object_size_;
          return s.get_data();
        }


        uint32_t object_size() const {
          return this->object_size_;
        }

      private:
        uint32_t object_size_;
      };

      class sync_looking_storage_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_looking_storage_response;

        sync_looking_storage_response(
          const const_data_buffer &object_id,
          const std::set<uint16_t> & replicas)
        : object_id_(object_id),
          replicas_(replicas) {
        }

        sync_looking_storage_response(
            binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->replicas_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->replicas_;
          return s.get_data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        const std::set<uint16_t> & replicas() const {
          return this->replicas_;
        }

      private:
        const_data_buffer object_id_;
        std::set<uint16_t> replicas_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_LOOKING_STORAGE_H_
