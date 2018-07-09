#ifndef __VDS_DHT_NETWORK_SYNC_coronation_H_
#define __VDS_DHT_NETWORK_SYNC_coronation_H_

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
      class sync_coronation_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_coronation_request;

        sync_coronation_request(
            const const_data_buffer &object_id,
            const const_data_buffer &source_node)
            : object_id_(object_id),
              source_node_(source_node) {
        }

        sync_coronation_request(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
      };

      class sync_coronation_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_coronation_response;

        sync_coronation_response(
            const const_data_buffer &object_id,
            const const_data_buffer &target_node,
	    uint64_t generation_id)
            : object_id_(object_id),
              source_node_(source_node) {
        }

        sync_coronation_response(
            binary_deserializer & s) {
          s
              >> this->object_id_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->object_id_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & object_id() const {
          return object_id_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer object_id_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_REPLICA_REQUEST_H_
