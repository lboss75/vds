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
          const const_data_buffer & object_id,
          const std::set<uint16_t> & exist_replicas)
          : source_node_id_(source_node_id),
            object_id_(object_id),
            exist_replicas_(exist_replicas){
      }

      chunk_query_replica(
          binary_deserializer & s) {
        s >> this->source_node_id_ >> this->object_id_ >> this->exist_replicas_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_id_ << this->object_id_ << this->exist_replicas_;
        return s.data();
      }


      const guid & source_node_id() const {
        return source_node_id_;
      }

      const std::set<uint16_t> & exist_replicas() const {
        return exist_replicas_;
      }

      const const_data_buffer & object_id() const {
        return object_id_;
      }

    private:
      guid source_node_id_;
      const_data_buffer object_id_;
      std::set<uint16_t> exist_replicas_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_QUERY_REPLICA_H_
