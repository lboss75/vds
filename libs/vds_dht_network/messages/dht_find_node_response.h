#ifndef __VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_
#define __VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_

#include "p2p_message_id.h"

namespace vds {
  namespace p2p_messages {
    class dht_find_node_response {
    public:
      static const network::message_type_t message_id = p2p_message_id::dht_find_node_response;

      struct target_node {
        guid target_id_;
        std::string address_;

        target_node() {
        }

        target_node(
          const guid & target_id,
          const std::string & address)
        : target_id_(target_id), address_(address){          
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
        s << message_id << this->nodes_;
        return s.data();
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
    const vds::p2p_messages::dht_find_node_response::target_node & node) {
  return s << node.target_id_ << node.address_;
}

inline vds::binary_deserializer & operator >> (
    vds::binary_deserializer & s,
    vds::p2p_messages::dht_find_node_response::target_node & node) {
  return s >> node.target_id_ >> node.address_;
}

#endif //__VDS_P2P_NETWORK_DHT_FIND_NODE_RESPONSE_H_
