#ifndef __VDS_DHT_NETWORK_REPLICA_REQUEST_H_
#define __VDS_DHT_NETWORK_REPLICA_REQUEST_H_

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
      class replica_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::replica_request;

        replica_request(
            const const_data_buffer &object_id,
            const std::vector<uint16_t> & replicas,
            const const_data_buffer &source_node)
            : object_id_(object_id),
              replicas_(replicas),
              source_node_(source_node) {
        }

        replica_request(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->replicas_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->replicas_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        const std::vector<uint16_t> & replicas() const {
          return replicas_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        std::vector<uint16_t> replicas_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_REPLICA_REQUEST_H_
