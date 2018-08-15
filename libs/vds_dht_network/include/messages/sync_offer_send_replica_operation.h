#ifndef __VDS_DHT_NETWORK_SYNC_OFFER_SEND_REPLICA_OPERATION_H_
#define __VDS_DHT_NETWORK_SYNC_OFFER_SEND_REPLICA_OPERATION_H_

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
      class sync_offer_send_replica_operation_request : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_offer_send_replica_operation_request;

        sync_offer_send_replica_operation_request(
          const const_data_buffer &object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          uint16_t replica,
          const const_data_buffer &target_node)
          : sync_base_message_request(
            object_id,
            generation,
            current_term,
            commit_index,
            last_applied),
          replica_(replica),
          target_node_(target_node){
        }

        sync_offer_send_replica_operation_request(
          binary_deserializer & s)
          : sync_base_message_request(s) {
          s
            >> this->replica_
            >> this->target_node_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
            << this->replica_
            << this->target_node_
            ;
          return s.get_data();
        }
        
        uint16_t replica() const {
          return this->replica_;
        }

        const const_data_buffer & target_node() const {
          return  this->target_node_;
        }
      private:
        uint16_t replica_;
        const_data_buffer target_node_;
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

#endif //__VDS_DHT_NETWORK_SYNC_OFFER_SEND_REPLICA_OPERATION_H_
