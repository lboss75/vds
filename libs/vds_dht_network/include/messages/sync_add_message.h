#ifndef __VDS_DHT_NETWORK_SYNC_ADD_MESSAGE_H_
#define __VDS_DHT_NETWORK_SYNC_ADD_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "sync_message_dbo.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_add_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_add_message_request;

        sync_add_message_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          const const_data_buffer &source_node,
          uint64_t local_index,
          orm::sync_message_dbo::message_type_t message_type,
          const const_data_buffer &member_node,
          uint16_t replica)
        : object_id_(object_id),
          leader_node_(leader_node),
          source_node_(source_node),
          local_index_(local_index),
          message_type_(message_type),
          member_node_(member_node),
          replica_(replica) {
        }

        sync_add_message_request(
            binary_deserializer & s) {
          uint8_t message_type;
          s
            >> this->object_id_
            >> this->leader_node_
            >> this->source_node_
            >> this->local_index_
            >> message_type
            >> this->member_node_
            >> this->replica_
          ;
          this->message_type_ = static_cast<orm::sync_message_dbo::message_type_t>(message_type);
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->leader_node_
            << this->source_node_
            << this->local_index_
            << static_cast<uint8_t>(this->message_type_)
            << this->member_node_
            << this->replica_
            ;
            return s.get_data();
        }


        const const_data_buffer& object_id() const {
          return this->object_id_;
        }

        const const_data_buffer& leader_node() const {
          return this->leader_node_;
        }

        const const_data_buffer& source_node() const {
          return this->source_node_;
        }

        const uint64_t& local_index() const {
          return this->local_index_;
        }

        const orm::sync_message_dbo::message_type_t& message_type() const {
          return this->message_type_;
        }

        const const_data_buffer &member_node() const {
          return this->member_node_;
        }

        const uint16_t& replica() const {
          return this->replica_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer leader_node_;
        const_data_buffer source_node_;
        uint64_t local_index_;
        orm::sync_message_dbo::message_type_t message_type_;
        const_data_buffer member_node_;
        uint16_t replica_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_ADD_MESSAGE_H_
