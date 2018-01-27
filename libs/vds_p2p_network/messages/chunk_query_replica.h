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
          const const_data_buffer & block_id,
          const std::list<uint16_t > & replicas)
          : source_node_id_(source_node_id),
            block_id_(block_id),
            replicas_(replicas){
      }

      chunk_query_replica(
          binary_deserializer & s) {
        s >> this->source_node_id_ >> this->block_id_ >> this->replicas_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_id_ << this->block_id_ << this->replicas_;
        return s.data();
      }

    private:
      guid source_node_id_;
      const_data_buffer block_id_;
      std::list<uint16_t> replicas_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_QUERY_REPLICA_H_
