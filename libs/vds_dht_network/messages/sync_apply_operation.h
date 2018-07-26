#ifndef __VDS_DHT_NETWORK_SYNC_APPLY_OPERATION_H_
#define __VDS_DHT_NETWORK_SYNC_APPLY_OPERATION_H_

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
      class sync_apply_operation_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_apply_operation_request;

        sync_apply_operation_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          const const_data_buffer &source_node)
            : sync_base_message_request(
              object_id,
              leader_node,
              generation,
              current_term,
              commit_index,
              last_applied),
          source_node_(source_node) {
        }

        sync_apply_operation_request(
            binary_deserializer & s)
        : sync_base_message_request(s) {
          s >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s << this->source_node_;
          return s.data();
        }

        const const_data_buffer & source_node() const override {
          return this->source_node_;
        }
      private:
        const_data_buffer source_node_;
      };

      class sync_apply_operation_response : public sync_base_message_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_apply_operation_response;

        sync_apply_operation_response(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          const const_data_buffer &source_node)
            : sync_base_message_response(object_id, leader_node, generation, current_term, commit_index, last_applied, source_node) {
        }

        sync_apply_operation_response(
            binary_deserializer & s)
        : sync_base_message_response(s) {
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_response::serialize(s);
          return s.data();
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_APPLY_OPERATION_H_
