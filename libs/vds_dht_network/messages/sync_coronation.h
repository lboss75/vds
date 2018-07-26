#ifndef __VDS_DHT_NETWORK_SYNC_CORONATION_H_
#define __VDS_DHT_NETWORK_SYNC_CORONATION_H_

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
      class sync_coronation_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_coronation_request;

        sync_coronation_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          const std::map<const_data_buffer, std::set<uint16_t>> & member_notes)
        : sync_base_message_request(object_id, leader_node, generation, current_term, commit_index, last_applied),
              member_notes_(member_notes) {
        }

        sync_coronation_request(binary_deserializer & s)
        :  sync_base_message_request(s) {
          s
              >> this->member_notes_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
            << this->member_notes_
            ;
          return s.data();
        }

        const std::map<const_data_buffer, std::set<uint16_t>> & member_notes() const {
          return this->member_notes_;
        }

        const const_data_buffer & source_node() const override {
          return this->leader_node();
        }


      private:
        std::map<const_data_buffer, std::set<uint16_t>> member_notes_;
      };

      class sync_coronation_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_coronation_response;

        sync_coronation_response(
            const const_data_buffer &object_id,
            uint64_t current_term,
            const const_data_buffer &source_node)
            : object_id_(object_id),
              current_term_(current_term),
              source_node_(source_node) {
        }

        sync_coronation_response(
            binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->current_term_
            >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->current_term_
            << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        uint64_t generation() const {
          return this->generation_;
        }

        uint64_t current_term() const {
          return this->current_term_;
        }

        const const_data_buffer & source_node() const {
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

#endif //__VDS_DHT_NETWORK_SYNC_CORONATION_H_
