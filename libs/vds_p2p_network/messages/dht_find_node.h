#ifndef __VDS_P2P_NETWORK_DHT_FIND_NODE_H_
#define __VDS_P2P_NETWORK_DHT_FIND_NODE_H_

#include "p2p_message_id.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class dht_find_node {
    public:
      static const uint8_t message_id = (uint8_t) p2p_message_id::dht_find_node;

      dht_find_node(
          const const_data_buffer & target_id)
      : target_id_(target_id) {
      }

      dht_find_node(
          binary_deserializer & s) {
        s >> this->target_id_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->target_id_;
        return s.data();
      }


      const const_data_buffer & target_id() const {
        return target_id_;
      }

    private:
      const_data_buffer target_id_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_DHT_FIND_NODE_H_
