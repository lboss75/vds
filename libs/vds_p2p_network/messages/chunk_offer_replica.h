#ifndef __VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_
#define __VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"
#include "guid.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class chunk_offer_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_offer_replica;

      chunk_offer_replica(
          size_t distance,
          const guid &source_node_id,
          const const_data_buffer & data_hash)
          : distance_(distance),
            source_node_id_(source_node_id),
            data_hash_(data_hash) {
      }

      chunk_offer_replica(
          binary_deserializer & s) {
        s >> this->distance_ >> this->source_node_id_ >> this->data_hash_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->distance_ << this->source_node_id_ << this->data_hash_;
        return s.data();
      }

      size_t distance() const {
        return this->distance_;
      }

      const const_data_buffer & data_hash() const {
        return this->data_hash_;
      }

      const guid &source_node_id() const {
        return this->source_node_id_;
      }

    private:
      size_t distance_;
      guid source_node_id_;
      const_data_buffer data_hash_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_
