#ifndef __VDS_P2P_NETWORK_DHT_FIND_NODE_H_
#define __VDS_P2P_NETWORK_DHT_FIND_NODE_H_

#include "dht_message_type.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class dht_find_node {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_find_node;

        dht_find_node(
          const const_data_buffer & target_id,
          const const_data_buffer & source_node)
          : target_id_(target_id),
          source_node_(source_node){
        }

        dht_find_node(
          binary_deserializer & s) {
          s >> this->target_id_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->target_id_;
          return s.data();
        }


        const const_data_buffer & target_id() const {
          return target_id_;
        }

      private:
        const_data_buffer target_id_;
        const_data_buffer source_node_;
      };
    }
  }
}
#endif //__VDS_P2P_NETWORK_DHT_FIND_NODE_H_
