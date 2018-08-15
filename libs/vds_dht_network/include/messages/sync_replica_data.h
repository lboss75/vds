#ifndef __VDS_DHT_NETWORK_REPLICA_DATA_H_
#define __VDS_DHT_NETWORK_REPLICA_DATA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "sync_base_message.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_replica_data : public sync_base_message_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_replica_data;

        sync_replica_data(
            const const_data_buffer &object_id,
            const const_data_buffer &leader_node,
            uint64_t generation,
            uint64_t current_term,
            uint64_t commit_index,
            uint64_t last_applied,
            uint16_t replica,
            const const_data_buffer & data)
            : sync_base_message_request(
              object_id,
              generation,
              current_term,
              commit_index,
              last_applied),
              replica_(replica),
              data_(data),
              leader_node_(leader_node) {
        }

        sync_replica_data(
            binary_deserializer & s)
        : sync_base_message_request(s){
          s
              >> this->replica_
              >> this->data_
              >> this->leader_node_
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_request::serialize(s);
          s
              << this->replica_
              << this->data_
              << this->leader_node_
          ;
          return s.get_data();
        }

        uint16_t replica() const {
          return this->replica_;
        }

        const const_data_buffer & data() const {
          return this->data_;
        }

        const const_data_buffer & leader_node() const {
          return this->leader_node_;
        }
      private:
        uint16_t replica_;
        const_data_buffer data_;
        const_data_buffer leader_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_REPLICA_DATA_H_
