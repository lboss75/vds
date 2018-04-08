#ifndef __VDS_P2P_NETWORK_DHT_PONG_H_
#define __VDS_P2P_NETWORK_DHT_PONG_H_

#include "dht_message_type.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class dht_pong {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_pong;

        dht_pong(
          const const_data_buffer & target_node,
          const const_data_buffer & source_node)
          : target_node_(target_node),
          source_node_(source_node) {
        }

        dht_pong(
          binary_deserializer & s) {
          s >> this->target_node_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->target_node_ << this->source_node_;
          return s.data();
        }

        const const_data_buffer & target_node() const {
          return target_node_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer target_node_;
        /**
        * \brief Ping initiator
        */
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_DHT_PONG_H_
