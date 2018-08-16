#ifndef __VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_
#define __VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_

#include <list>

#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class dht_find_node_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::dht_find_node_response;

        struct target_node {
          const_data_buffer target_id_;
          std::string address_;
          uint8_t hops_;

          target_node() {
          }

          target_node(
            const const_data_buffer & target_id,
            const std::string & address,
            uint8_t hops)
            : target_id_(target_id), address_(address), hops_(hops) {
          }
        };

        dht_find_node_response(
          const std::list<target_node> & nodes)
          : nodes_(nodes) {
        }

        dht_find_node_response(
          binary_deserializer & s) {
          s >> this->nodes_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->nodes_;
          return s.move_data();
        }


        const std::list<target_node> & nodes() const {
          return nodes_;
        }

      private:
        std::list<target_node> nodes_;
      };
    }
  }
inline vds::binary_serializer & operator << (
    vds::binary_serializer & s,
    const vds::dht::messages::dht_find_node_response::target_node & node) {
  return s << node.target_id_ << node.address_ << node.hops_;
}

inline vds::binary_deserializer & operator >> (
    vds::binary_deserializer & s,
    vds::dht::messages::dht_find_node_response::target_node & node) {
  return s >> node.target_id_ >> node.address_ >> node.hops_;
}
}


#endif //__VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_
