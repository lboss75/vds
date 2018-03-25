#ifndef __VDS_DHT_NETWORK_REPLICA_NOT_FOUND_H_
#define __VDS_DHT_NETWORK_REPLICA_NOT_FOUND_H_

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
      class replica_not_found {
      public:
        static const uint8_t message_id = (uint8_t)network::message_type_t::replica_not_found;

        replica_not_found(
            const const_data_buffer &object_id,
            uint16_t replica,
            const const_data_buffer &source_node)
            : object_id_(object_id_), replica_(replica_),
              source_node_(source_node_) {
        }

        replica_not_found(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->replica_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->replica_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        uint16_t replica() const {
          return replica_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        uint16_t replica_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_REPLICA_NOT_FOUND_H_
