#ifndef __VDS_DHT_NETWORK_OFFER_REPLICA_H_
#define __VDS_DHT_NETWORK_OFFER_REPLICA_H_

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
      class offer_replica {
      public:
        static const network::message_type_t message_id = network::message_type_t::offer_replica;

        offer_replica(
          const const_data_buffer &target_node,
          const const_data_buffer &object_id,
          uint16_t replica,
          const const_data_buffer &replica_data,
          const const_data_buffer &adviser,
          const const_data_buffer &source_node)
            : object_id_(object_id), replica_(replica), replica_data_(replica_data), adviser_(adviser),
              source_node_(source_node) {
        }

        offer_replica(
          binary_deserializer & s) {
          s
              >> this->target_node_
              >> this->object_id_
              >> this->replica_
              >> this->replica_data_
              >> this->adviser_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->target_node_
              << this->object_id_
              << this->replica_
              << this->replica_data_
              << this->adviser_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & target_node() const {
          return this->target_node_;
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        uint16_t replica() const {
          return replica_;
        }

        const const_data_buffer & replica_data() const {
          return replica_data_;
        }

        const const_data_buffer & adviser() const {
          return adviser_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer target_node_;
        const_data_buffer object_id_;
        uint16_t replica_;
        const_data_buffer replica_data_;
        const_data_buffer adviser_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_OFFER_REPLICA_H_
