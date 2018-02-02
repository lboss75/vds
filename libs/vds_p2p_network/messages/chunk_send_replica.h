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
        const const_data_buffer & block_id,
        const uint16_t replica,
        const const_data_buffer& replica_data,
        const const_data_buffer& replica_hash)
        : block_id_(block_id),
          replica_(replica),
          replica_data_(replica_data),
          replica_hash_(replica_hash) {
      }

      chunk_send_replica(
          binary_deserializer & s) {
        s >> this->block_id_ >> this->replica_ >> this->replica_data_ >> this->replica_hash_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->block_id_ << this->replica_ << this->replica_data_ << this->replica_hash_;
        return s.data();
      }


      const const_data_buffer & block_id() const {
        return block_id_;
      }

      uint16_t replica() const {
        return replica_;
      }

      const const_data_buffer & replica_data() const {
        return replica_data_;
      }

      const const_data_buffer & replica_hash() const {
        return replica_hash_;
      }

    private:
      const_data_buffer block_id_;
      uint16_t replica_;
      const_data_buffer replica_data_;
      const_data_buffer replica_hash_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_SEND_REPLICA_H_
