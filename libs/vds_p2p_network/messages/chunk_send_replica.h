#ifndef __VDS_P2P_NETWORK_SEND_REPLICA_H_
#define __VDS_P2P_NETWORK_SEND_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"

namespace vds {
  namespace p2p_messages {
    class chunk_send_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_send_replica;

      chunk_send_replica(
          const guid &source_node_id,
          const const_data_buffer &body)
          : source_node_id_(source_node_id),
            body_(body) {
      }

      chunk_send_replica(
          binary_deserializer & s) {
        s >> this->source_node_id_ >> this->body_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_id_ << this->body_;
        return s.data();
      }

    private:
      guid source_node_id_;
      const_data_buffer body_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_SEND_REPLICA_H_
