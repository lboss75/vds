#ifndef __VDS_P2P_NETWORK_DHT_PING_H_
#define __VDS_P2P_NETWORK_DHT_PING_H_

#include "dht_message_type.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class dht_ping {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_ping;

        dht_ping() {
        }

        dht_ping(
          binary_deserializer& s) {
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          return s.move_data();
        }
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_DHT_PING_H_
