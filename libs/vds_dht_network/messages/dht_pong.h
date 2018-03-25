#ifndef __VDS_P2P_NETWORK_DHT_PONG_H_
#define __VDS_P2P_NETWORK_DHT_PONG_H_

#include "p2p_message_id.h"

namespace vds {
  namespace p2p_messages {
    class dht_pong {
    public:
      static const network::message_type_t message_id = p2p_message_id::dht_pong;

      dht_pong(const guid & source_node)
      : source_node_(source_node){
      }

      dht_pong(
          binary_deserializer & s) {
        s >> this->source_node_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->source_node_;
        return s.data();
      }

      const guid & source_node() const {
        return source_node_;
      }

    private:

      /**
       * \brief Ping receiver
       */
      guid source_node_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_DHT_PONG_H_
