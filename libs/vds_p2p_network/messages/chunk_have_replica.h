#ifndef __VDS_P2P_NETWORK_HAVE_REPLICA_H_
#define __VDS_P2P_NETWORK_HAVE_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "p2p_message_id.h"
#include "guid.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    //// Dublicates
    class chunk_have_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_have_replica;

      chunk_have_replica(
          const guid &source_node_id,
          const const_data_buffer &object_id,
          const std::set<uint16_t> & replicas)
        : source_node_id_(source_node_id),
        object_id_(object_id),
        replicas_(replicas) {
      }

      chunk_have_replica(
        binary_deserializer & s) {
        s >> this->source_node_id_ >> this->object_id_ >> this->replicas_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_id_ << this->object_id_ << this->replicas_;
        return s.data();
      }

      const guid &source_node_id() const {
        return this->source_node_id_;
      }


      const const_data_buffer & object_id() const {
        return object_id_;
      }

      const std::set<uint16_t> & replicas() const {
        return replicas_;
      }
    private:
      guid source_node_id_;
      const_data_buffer object_id_;
      std::set<uint16_t> replicas_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_HAVE_REPLICA_H_
