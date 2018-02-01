#ifndef __VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_
#define __VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"

namespace vds {
  namespace p2p_messages {
    class chunk_offer_replica {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::chunk_offer_replica;

      chunk_offer_replica(
          const const_data_buffer & data_hash)
          : data_hash_(data_hash) {
      }

      chunk_offer_replica(
          binary_deserializer & s) {
        s >> this->data_hash_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->data_hash_;
        return s.data();
      }

      const const_data_buffer & data_hash() const {
        return this->data_hash_;
      }

    private:
      const_data_buffer data_hash_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_CHUNK_OFFER_REPLICA_H_
