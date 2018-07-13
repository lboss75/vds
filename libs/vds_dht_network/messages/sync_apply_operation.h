#ifndef __VDS_DHT_NETWORK_sync_apply_operation_H_
#define __VDS_DHT_NETWORK_SYNC_REPLICA_OPERATIONS_H_

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
      class sync_apply_operation_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_apply_operation_request;

        sync_apply_operation_request(
            const const_data_buffer &object_id,
            const const_data_buffer &source_node,
            uint64_t generation,
            uint64_t current_term,
            uint64_t commit_index,
            uint64_t last_applied)
            : object_id_(object_id),
              source_node_(source_node),
              generation_(generation),
              current_term_(current_term),
              commit_index_(commit_index),
              last_applied_(last_applied) {
        }

        sync_apply_operation_request(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->source_node_
              >> this->generation_
              >> this->current_term_
              >> this->commit_index_
              >> this->last_applied_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->source_node_
              << this->generation_
              << this->current_term_
              << this->commit_index_
              << this->last_applied_
            ;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

        uint64_t generation() const {
          return this->current_term_;
        }

        uint64_t current_term() const {
          return this->current_term_;
        }

        uint64_t commit_index() const {
          return this->commit_index_;
        }

        uint64_t last_applied() const {
          return this->last_applied_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
        uint64_t generation_;
        uint64_t current_term_;
        uint64_t commit_index_;
        uint64_t last_applied_;
      };

      class sync_apply_operation_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_apply_operation_response;

        sync_apply_operation_response(
            const const_data_buffer &object_id,
            const const_data_buffer &target_node,
	    uint64_t generation_id)
            : object_id_(object_id),
              source_node_(source_node) {
        }

        sync_apply_operation_response(
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

#endif //__VDS_DHT_NETWORK_SYNC_REPLICA_OPERATIONS_H_
