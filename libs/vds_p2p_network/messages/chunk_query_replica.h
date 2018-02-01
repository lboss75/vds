#ifndef __VDS_P2P_NETWORK_QUERY_REPLICA_H_
#define __VDS_P2P_NETWORK_QUERY_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "p2p_message_id.h"
#include "guid.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class chunk_query_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_query_replica;

      chunk_query_replica(
          const guid &source_node_id,
          const const_data_buffer & data_hash)
          : source_node_id_(source_node_id),
            data_hash_(data_hash){
      }

      chunk_query_replica(
          binary_deserializer & s) {
        s >> this->source_node_id_ >> this->data_hash_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_id_ << this->data_hash_;
        return s.data();
      }

    private:
      guid source_node_id_;
      const_data_buffer data_hash_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_QUERY_REPLICA_H_
