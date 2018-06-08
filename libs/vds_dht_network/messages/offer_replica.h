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
          const const_data_buffer &replica_hash,
          const const_data_buffer &source_node)
            : replica_hash_(replica_hash),
              source_node_(source_node) {
        }

        offer_replica(
          binary_deserializer & s) {
          s
              >> this->replica_hash_
              >> this->source_node_;
          ;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
              << this->replica_hash_
              << this->source_node_;
          return s.data();
        }

        const const_data_buffer & replica_hash() const {
          return replica_hash_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer replica_hash_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_OFFER_REPLICA_H_
