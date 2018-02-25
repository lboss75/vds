#ifndef __VDS_P2P_NETWORK_RAFT_ADD_CLIENT_H_
#define __VDS_P2P_NETWORK_RAFT_ADD_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include "private/node_id_t.h"
#include "p2p_message_id.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "guid.h"
#include "private/node_id_t.h"

namespace vds {
  namespace p2p_messages {
    class raft_add_client {
    public:
      static constexpr uint8_t message_id = (uint8_t)p2p_message_id::raft_get_lead;

      raft_add_client(
          const guid & channel_id,
          const node_id_t & sender_id,
          const std::set<node_id_t> & nodes)
          : channel_id_(channel_id),
            sender_id_(sender_id),
            nodes_(nodes){
      }

      raft_add_client(
          binary_deserializer & s) {
        s >> this->channel_id_ >> this->sender_id_ >> this->nodes_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->channel_id_ << this->sender_id_ << this->nodes_;
        return s.data();
      }

      const guid & channel_id() const {
        return channel_id_;
      }

      const node_id_t & sender_id() const {
        return sender_id_;
      }

      const std::set<node_id_t> & nodes() const {
        return nodes_;
      }

    private:
      guid channel_id_;
      node_id_t sender_id_;
      std::set<node_id_t> nodes_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_RAFT_ADD_CLIENT_H_
