#ifndef __VDS_DHT_NETWORK_SYNC_NEW_ELECTION_H_
#define __VDS_DHT_NETWORK_SYNC_NEW_ELECTION_H_

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
      class sync_new_election_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_new_election_request;

        sync_new_election_request(
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          const const_data_buffer& source_node)
          : object_id_(object_id),
            generation_(generation),
            current_term_(current_term),
            source_node_(source_node) {
        }

        sync_new_election_request(
          binary_deserializer& s) {
          s
            >> this->object_id_
            >> this->generation_
            >> this->current_term_
            >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->generation_
            << this->current_term_
            << this->source_node_;
          return s.move_data();
        }

        const const_data_buffer& object_id() const {
          return this->object_id_;
        }

        uint64_t generation() const {
          return this->generation_;
        }

        uint64_t current_term() const {
          return this->current_term_;
        }

        const const_data_buffer& source_node() const {
          return this->source_node_;
        }

      private:
        const_data_buffer object_id_;
        uint64_t generation_;
        uint64_t current_term_;
        const_data_buffer source_node_;
      };

      class sync_new_election_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_new_election_response;

        sync_new_election_response(
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          const const_data_buffer& source_node)
          : object_id_(object_id),
            generation_(generation),
            current_term_(current_term),
            source_node_(source_node) {
        }

        sync_new_election_response(
          binary_deserializer& s) {
          s
            >> this->object_id_
            >> this->generation_
            >> this->current_term_
            >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->generation_
            << this->current_term_
            << this->source_node_;
          return s.move_data();
        }

        const const_data_buffer& object_id() const {
          return object_id_;
        }

        uint64_t generation() const {
          return this->generation_;
        }

        uint64_t current_term() const {
          return this->current_term_;
        }

        const const_data_buffer& source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        uint64_t generation_;
        uint64_t current_term_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_NEW_ELECTION_H_
