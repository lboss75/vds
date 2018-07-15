#ifndef __VDS_DHT_NETWORK_SYNC_MEMBER_OPERATION_H_
#define __VDS_DHT_NETWORK_SYNC_MEMBER_OPERATION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_member_operation_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_member_operation_request;

        enum class operation_type_t : uint8_t {
          add_member,
          remove_member
        };

        sync_member_operation_request(
            const const_data_buffer &object_id,
            const const_data_buffer &source_node,
            uint64_t generation,
            uint64_t current_term,
            operation_type_t operation_type)
            : object_id_(object_id),
              source_node_(source_node),
              operation_type_(operation_type) {
        }

        sync_member_operation_request(
            binary_deserializer & s) {
          uint8_t operation_type;
          s
              >> this->object_id_
              >> this->source_node_
              >> this->generation_
              >> this->current_term_
              >> operation_type;

          this->operation_type_ = static_cast<operation_type_t>(operation_type);
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->source_node_
            << this->generation_
            << this->current_term_
            << static_cast<uint8_t>(this->operation_type_);
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        const const_data_buffer & source_node() const {
          return this->source_node_;
        }

        uint64_t generation() const {
          return this->generation_;
        }

        uint64_t current_term() const {
          return this->current_term_;
        }

        operation_type_t operation_type() const {
          return this->operation_type_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
        uint64_t generation_;
        uint64_t current_term_;
        operation_type_t operation_type_;
      };

      class sync_member_operation_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_member_operation_response;

        sync_member_operation_response(
            const const_data_buffer &object_id,
            const const_data_buffer &source_node,
	          uint64_t generation_id)
            : object_id_(object_id),
              source_node_(source_node) {
        }

        sync_member_operation_response(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_MEMBER_OPERATION_H_
