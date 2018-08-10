#ifndef __VDS_DHT_NETWORK_SYNC_REPLICA_OPERATIONS_H_
#define __VDS_DHT_NETWORK_SYNC_REPLICA_OPERATIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "messages/sync_base_message.h"
#include "sync_message_dbo.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_replica_operations_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_replica_operations_request;

        sync_replica_operations_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer & member_node,
          uint16_t replica,
          const const_data_buffer & message_source_node,
          uint64_t message_source_index,
          const const_data_buffer & target_node)
            : sync_base_message_request(
              object_id,
              leader_node,
              generation,
              current_term,
              commit_index,
              last_applied),
        message_type_(message_type),
        member_node_(member_node),
        replica_(replica),
          message_source_node_(message_source_node),
        message_source_index_(message_source_index),
        target_node_(target_node) {
        }

        sync_replica_operations_request(
            binary_deserializer & s)
        : sync_base_message_request(s) {
          uint8_t message_type;
          s
            >> message_type
            >> this->member_node_
            >> this->replica_
            >> this->message_source_node_
            >> this->message_source_index_
            >> this->target_node_
            ;

          this->message_type_ = static_cast<orm::sync_message_dbo::message_type_t>(message_type);
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
            << static_cast<uint8_t>(this->message_type_)
            << this->member_node_
            << this->replica_
            << this->message_source_node_
            << this->message_source_index_
            << this->target_node_
            ;
          return s.data();
        }
        orm::sync_message_dbo::message_type_t message_type() const {
          return this->message_type_;
        }
        
        const const_data_buffer & member_node() const {
          return this->member_node_;
        }

        uint16_t replica() const {
          return this->replica_;
        }

        const const_data_buffer & source_node() const override {
          return this->leader_node();
        }

        const const_data_buffer& target_node() const {
          return this->target_node_;
        }

        const const_data_buffer & message_source_node() const {
          return this->message_source_node_;
        }

        uint64_t message_source_index() const {
          return this->message_source_index_;
        }

      private:
        orm::sync_message_dbo::message_type_t message_type_;
        const_data_buffer member_node_;
        uint16_t replica_;
        const_data_buffer message_source_node_;
        uint64_t message_source_index_;
        const_data_buffer target_node_;
      };

      class sync_replica_operations_response : public sync_base_message_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_replica_operations_response;

        sync_replica_operations_response(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          const const_data_buffer &source_node)
            : sync_base_message_response(
              object_id,
              leader_node,
              generation,
              current_term,
              commit_index,
              last_applied,
              source_node) {
        }

        sync_replica_operations_response(
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

#endif //__VDS_DHT_NETWORK_SYNC_REPLICA_OPERATIONS_H_