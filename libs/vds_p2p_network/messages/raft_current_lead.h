#ifndef __VDS_P2P_NETWORK_RAFT_CURRENT_LEAD_H_
#define __VDS_P2P_NETWORK_RAFT_CURRENT_LEAD_H_

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
    class raft_current_lead {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::raft_current_lead;

      raft_current_lead(
          const guid & channel_id,
          const node_id_t & leader_id)
          : channel_id_(channel_id),
            leader_id_(leader_id){
      }

      raft_current_lead(
          binary_deserializer & s) {
        s >> this->channel_id_ >> this->leader_id_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->channel_id_ << this->leader_id_;
        return s.data();
      }

      const guid & channel_id() const {
        return channel_id_;
      }

      const node_id_t & leader_id() const {
        return leader_id_;
      }

    private:
      guid channel_id_;

      guid generation_id_;
      node_id_t leader_id_;
      size_t current_term_;
      size_t commit_index_;
      size_t last_applied_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_RAFT_CURRENT_LEAD_H_
