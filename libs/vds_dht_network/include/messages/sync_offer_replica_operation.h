#ifndef __VDS_DHT_NETWORK_SYNC_OFFER_REPLICA_OPERATION_H_
#define __VDS_DHT_NETWORK_SYNC_OFFER_REPLICA_OPERATION_H_

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
      class sync_offer_replica_operation_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_offer_replica_operation_request;

        enum class message_type_t : uint8_t {
          add_replica,
          move_replica,
          remove_replica
        };

        sync_offer_replica_operation_request(
          const const_data_buffer &object_id,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          message_type_t message_type,
          const const_data_buffer & member_node,
          uint16_t replica)
          : sync_base_message_request(
            object_id,
            leader_node,
            generation,
            current_term,
            commit_index,
            last_applied),
          message_type_(message_type),
          member_node_(member_node),
          replica_(replica) {
        }

        sync_offer_replica_operation_request(
          binary_deserializer & s)
          : sync_base_message_request(s) {
          uint8_t message_type;
          s
            >> message_type
            >> this->member_node_
            >> this->replica_;

          this->message_type_ = static_cast<message_type_t>(message_type);
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
            << static_cast<uint8_t>(this->message_type_)
            << this->member_node_
            << this->replica_;
          return s.data();
        }
        
        message_type_t message_type() const {
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

      private:
        message_type_t message_type_;
        const_data_buffer member_node_;
        uint16_t replica_;
      };

      //class sync_offer_replica_operation_response : public sync_base_message_response {
      //public:
      //  static const network::message_type_t message_id = network::message_type_t::sync_offer_replica_operation_response;

      //  enum class message_type_t : uint8_t {
      //    get_replica,
      //  };

      //  sync_offer_replica_operation_response(
      //    const const_data_buffer &object_id,
      //    const const_data_buffer &leader_node,
      //    uint64_t generation,
      //    uint64_t current_term,
      //    uint64_t commit_index,
      //    uint64_t last_applied,
      //    message_type_t message_type,
      //    const const_data_buffer & member_node,
      //    uint16_t replica)
      //    : sync_base_message_response(
      //      object_id,
      //      leader_node,
      //      generation,
      //      current_term,
      //      commit_index,
      //      last_applied),
      //    message_type_(message_type),
      //    member_node_(member_node),
      //    replica_(replica) {
      //  }

      //  sync_offer_replica_operation_response(
      //    binary_deserializer & s)
      //    : sync_base_message_response(s) {
      //    uint8_t message_type;
      //    s
      //      >> message_type
      //      >> this->member_node_
      //      >> this->replica_;

      //    this->message_type_ = static_cast<message_type_t>(message_type);
      //  }

      //  const_data_buffer serialize() const {
      //    binary_serializer s;
      //    sync_base_message_response::serialize(s);
      //    s
      //      << static_cast<uint8_t>(this->message_type_)
      //      << this->member_node_
      //      << this->replica_;
      //    return s.data();
      //  }

      //  message_type_t message_type() const {
      //    return this->message_type_;
      //  }

      //  const const_data_buffer & member_node() const {
      //    return this->member_node_;
      //  }

      //  uint16_t replica() const {
      //    return this->replica_;
      //  }

      //private:
      //  message_type_t message_type_;
      //  const_data_buffer member_node_;
      //  uint16_t replica_;
      //};
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_OFFER_REPLICA_OPERATION_H_
