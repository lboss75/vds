#ifndef __VDS_DHT_NETWORK_REPLICA_DATA_H_
#define __VDS_DHT_NETWORK_REPLICA_DATA_H_

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
      class replica_data {
      public:
        static const network::message_type_t message_id = network::message_type_t::replica_data;

        replica_data(
            const const_data_buffer &object_id,
            uint16_t replica,
            const const_data_buffer & data,
            const const_data_buffer &source_node)
            : object_id_(object_id),
              replica_(replica),
              data_(data),
              source_node_(source_node) {
        }

        replica_data(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->replica_
              >> this->data_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->replica_
              << this->data_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        uint16_t replica() const {
          return this->replica_;
        }

        const const_data_buffer & data() const {
          return this->data_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        uint16_t replica_;
        const_data_buffer data_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_REPLICA_DATA_H_
