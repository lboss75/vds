#ifndef __VDS_P2P_NETWORK_DHT_PING_H_
#define __VDS_P2P_NETWORK_DHT_PING_H_

#include "p2p_message_id.h"

namespace vds {
  namespace p2p_messages {
    class dht_ping {
    public:
      static const uint8_t message_id = (uint8_t) p2p_message_id::dht_ping;

      dht_ping(
          const const_data_buffer & target_id)
      : target_id_(target_id) {
      }

      dht_ping(
          binary_deserializer & s) {
        s >> this->target_id_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->target_id_;
        return s.data();
      }

    private:
      const_data_buffer target_id_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_DHT_PING_H_
