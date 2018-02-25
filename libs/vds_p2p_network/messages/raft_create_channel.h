#ifndef __VDS_P2P_NETWORK_RAFT_CREATE_CHANNEL_H_
#define __VDS_P2P_NETWORK_RAFT_CREATE_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <private/node_id_t.h>
#include "p2p_message_id.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "guid.h"
#include "private/node_id_t.h"

namespace vds {
  namespace p2p_messages {
    class raft_create_channel {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::raft_current_lead;

      raft_create_channel(
          const guid & channel_id,
          const node_id_t & sender_id,
          std::list<certificate> owner_certificate,
          const std::set<node_id_t> & nodes)
          : channel_id_(channel_id),
            sender_id_(sender_id),
            owner_certificate_(owner_certificate),
            nodes_(nodes) {
      }

      raft_create_channel(
          binary_deserializer & s) {
        s
            >> this->channel_id_
            >> this->sender_id_
            >> this->owner_certificate_
            >> this->nodes_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s
            << this->channel_id_
            << this->sender_id_
            << this->owner_certificate_
            << this->nodes_;
        return s.data();
      }

      const guid & channel_id() const {
        return channel_id_;
      }

      const node_id_t &sender_id() const {
        return sender_id_;
      }

      const std::list<certificate> & owner_certificate() const {
        return owner_certificate_;
      }

      const std::set<node_id_t> & nodes() const {
        return nodes_;
      }

    private:
      guid channel_id_;
      node_id_t sender_id_;
      std::list<certificate> owner_certificate_;
      std::set<node_id_t> nodes_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_RAFT_CREATE_CHANNEL_H_
