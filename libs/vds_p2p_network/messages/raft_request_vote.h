#ifndef __VDS_P2P_NETWORK_RAFT_REQUEST_VOTE_H_
#define __VDS_P2P_NETWORK_RAFT_REQUEST_VOTE_H_

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
    class raft_request_vote {
    public:
      static const uint8_t message_id = (uint8_t) p2p_message_id::raft_request_vote;

      raft_request_vote(
          const guid & channel_id,
          size_t current_term,
          size_t last_log_idx,
          size_t last_log_term,
          const node_id_t &node_id)
          : channel_id_(channel_id),
            current_term_(current_term),
            last_log_idx_(last_log_idx),
            last_log_term_(last_log_term),
            node_id_(node_id) {
      }

      raft_request_vote(
          binary_deserializer &s) {
        s
            >> this->channel_id_
            >> this->current_term_
            >> this->last_log_idx_
            >> this->last_log_term_
            >> this->node_id_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s
            << this->channel_id_
            << this->current_term_
            << this->last_log_idx_
            << this->last_log_term_
            << this->node_id_;
        return s.data();
      }

      const guid &channel_id() const {
        return channel_id_;
      }

      size_t current_term() const {
        return current_term_;
      }

      size_t last_log_idx() const {
        return last_log_idx_;
      }

      size_t last_log_term() const {
        return last_log_term_;
      }

      const node_id_t & node_id() const {
        return node_id_;
      }

    private:
      guid channel_id_;
      size_t current_term_;
      size_t last_log_idx_;
      size_t last_log_term_;
      node_id_t node_id_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_RAFT_REQUEST_VOTE_H_
