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
    class chunk_have_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_have_replica;

      chunk_have_replica(
          //const guid &node_id,
          const const_data_buffer &data_hash)
          : //node_id_(node_id),
            data_hash_(data_hash) {
      }

      chunk_have_replica(
          binary_deserializer & s) {
        s >> this->data_hash_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->data_hash_;
        return s.data();
      }

      const guid & node_id() const {
        return node_id_;
      }

      const const_data_buffer & data_hash() const {
        return data_hash_;
      }

    private:
      guid node_id_;
      const_data_buffer data_hash_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_HAVE_REPLICA_H_
