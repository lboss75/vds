#ifndef __VDS_DHT_NETWORK_SYNC_SNAPSHOT_H_
#define __VDS_DHT_NETWORK_SYNC_SNAPSHOT_H_

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
    class sync_snapshot_request {
    public:
        static const network::message_type_t message_id = network::message_type_t::sync_snapshot_request;

        sync_snapshot_request(
            const const_data_buffer &object_id,
            const const_data_buffer &source_node)
            : object_id_(object_id),
              source_node_(source_node) {
        }

        sync_snapshot_request(
            binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->source_node_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->source_node_
          ;
          return s.move_data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        const const_data_buffer & source_node() const {
          return this->source_node_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
      };

      class sync_snapshot_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_snapshot_response;

        struct member_state {
          const_data_buffer voted_for;
          const_data_buffer cert;
          const_data_buffer sign;
        };

        sync_snapshot_response(
          const const_data_buffer &object_id,
          uint32_t object_size,
          const const_data_buffer &target_node,
          const const_data_buffer &leader_node,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied,
          const std::map<const_data_buffer, std::set<uint16_t>> & replica_map,
          const std::map<const_data_buffer, member_state> & members)
          : object_id_(object_id),
          object_size_(object_size),
          target_node_(target_node),
          leader_node_(leader_node),
          generation_(generation),
          current_term_(current_term),
          commit_index_(commit_index),
          last_applied_(last_applied),
          replica_map_(replica_map),
          members_(members){
        }

        sync_snapshot_response(
            binary_deserializer & s) {
          s
            >> this->object_id_
            >> this->object_size_
            >> this->target_node_
            >> this->leader_node_
            >> this->generation_
            >> this->current_term_
            >> this->commit_index_
            >> this->last_applied_
            >> this->replica_map_
            >> this->members_
            ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->object_id_
            << this->object_size_
            << this->target_node_
            << this->leader_node_
            << this->generation_
            << this->current_term_
            << this->commit_index_
            << this->last_applied_
            << this->replica_map_
            << this->members_
            ;
          return s.move_data();
        }

        const const_data_buffer & object_id() const {
          return this->object_id_;
        }

        uint32_t object_size() const {
          return this->object_size_;
        }

        const const_data_buffer & target_node() const {
          return this->leader_node_;
        }

        const const_data_buffer & leader_node() const {
          return this->leader_node_;
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

        const std::map<const_data_buffer, std::set<uint16_t>> & replica_map() const {
          return this->replica_map_;
        }

        const std::map<const_data_buffer, member_state> & members() const {
          return this->members_;
        }


      private:
        const_data_buffer object_id_;
        uint32_t object_size_;
        const_data_buffer target_node_;
        const_data_buffer leader_node_;
        uint64_t generation_;
        uint64_t current_term_;
        uint64_t commit_index_;
        uint64_t last_applied_;
        std::map<const_data_buffer, std::set<uint16_t>> replica_map_;
        std::map<const_data_buffer, member_state> members_;
      };
    }
  }

  inline binary_serializer & operator << (binary_serializer & s, const dht::messages::sync_snapshot_response::member_state & value) {
    s << value.voted_for << value.cert << value.sign;
    return s;
  }

  inline binary_deserializer & operator >> (binary_deserializer & s, dht::messages::sync_snapshot_response::member_state & value) {
    s >> value.voted_for >> value.cert >> value.sign;
    return s;
  }
}


#endif //__VDS_DHT_NETWORK_SYNC_SNAPSHOT_H_
