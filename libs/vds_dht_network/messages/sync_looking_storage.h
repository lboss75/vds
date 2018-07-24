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
      class sync_looking_storage_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_looking_storage_request;

        sync_looking_storage_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint32_t object_size)
        : object_id_(object_id),
          leader_node_(leader_node),
          object_size_(object_size){
        }

        sync_looking_storage_request(
            binary_deserializer & s){
          s
            >> this->object_id_
            >> this->leader_node_
            >> this->object_size_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->leader_node_
            << this->object_size_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        const const_data_buffer & leader_node() const {
          return this->leader_node_;
        }

        uint32_t object_size() const {
          return this->object_size_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer leader_node_;
        uint32_t object_size_;
      };

      class sync_looking_storage_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_looking_storage_response;

        sync_looking_storage_response(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          const const_data_buffer &source_node,
          const std::set<uint16_t> & replicas)
        : object_id_(object_id),
          leader_node_(leader_node),
          source_node_(source_node),
          replicas_(replicas) {
        }

        sync_looking_storage_response(
            binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->leader_node_
            >> this->source_node_
            >> this->replicas_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->leader_node_
            << this->source_node_
            << this->replicas_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        const const_data_buffer & leader_node() const {
          return this->leader_node_;
        }

        const const_data_buffer & source_node() const {
          return this->source_node_;          
        }

        const std::set<uint16_t> & replicas() const {
          return this->replicas_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer leader_node_;
        const_data_buffer source_node_;
        std::set<uint16_t> replicas_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_LOOKING_STORAGE_H_
