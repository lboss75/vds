#ifndef __VDS_DHT_NETWORK_SYNC_BASE_MESSAGE_H_
#define __VDS_DHT_NETWORK_SYNC_BASE_MESSAGE_H_

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
      class sync_base_message_request {
      public:
        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        uint64_t generation() const {
          return this->generation_;
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

      protected:
        sync_base_message_request(
          const const_data_buffer &object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied)
          : object_id_(object_id),
          generation_(generation),
          current_term_(current_term),
          commit_index_(commit_index),
          last_applied_(last_applied){
        }

        sync_base_message_request(
          binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->generation_
            >> this->current_term_
            >> this->commit_index_
            >> this->last_applied_
            ;
        }

        void serialize(binary_serializer & s) const {
          s
            << this->object_id_
            << this->generation_
            << this->current_term_
            << this->commit_index_
            << this->last_applied_
            ;
        }
      private:
        const_data_buffer object_id_;
        uint64_t generation_;
        uint64_t current_term_;
        uint64_t commit_index_;
        uint64_t last_applied_;
      };

      class sync_base_message_response {
      public:
        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        uint64_t generation() const {
          return this->generation_;
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

      protected:
        sync_base_message_response(
          const const_data_buffer &object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied)
          : object_id_(object_id),
          generation_(generation),
          current_term_(current_term),
          commit_index_(commit_index),
          last_applied_(last_applied){
        }

        sync_base_message_response(
          binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->generation_
            >> this->current_term_
            >> this->commit_index_
            >> this->last_applied_
            ;
        }

        void serialize(binary_serializer & s) const {
          s
            << this->object_id_
            << this->generation_
            << this->current_term_
            << this->commit_index_
            << this->last_applied_
            ;
        }
      private:
        const_data_buffer object_id_;
        uint64_t generation_;
        uint64_t current_term_;
        uint64_t commit_index_;
        uint64_t last_applied_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_BASE_MESSAGE_H_
