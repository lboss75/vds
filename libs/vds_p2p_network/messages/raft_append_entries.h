#ifndef __VDS_P2P_NETWORK_RAFT_APPEND_ENTRIES_H_
#define __VDS_P2P_NETWORK_RAFT_APPEND_ENTRIES_H_

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
    class raft_append_entries {
    public:
      static const uint8_t message_id = (uint8_t) p2p_message_id::raft_append_entries;

      raft_append_entries(
          const guid &channel_id,
          size_t current_term,
          size_t commit_idx,
          size_t last_log_idx,
          const std::list<const_data_buffer> &records)
          : channel_id_(channel_id),
            current_term_(current_term),
            commit_idx_(commit_idx),
            last_log_idx_(last_log_idx),
            records_(records) {
      }

      raft_append_entries(
          binary_deserializer &s) {
        s
            >> this->channel_id_
            >> this->current_term_
            >> this->commit_idx_
            >> this->last_log_idx_
            >> this->records_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s
            << this->channel_id_
            << this->current_term_
            << this->commit_idx_
            << this->last_log_idx_
            << this->records_;
        return s.data();
      }

      const guid &channel_id() const {
        return channel_id_;
      }

      size_t current_term() const {
        return current_term_;
      }

      size_t commit_idx() const {
        return commit_idx_;
      }

      size_t last_log_idx() const {
        return last_log_idx_;
      }

      const std::list<const_data_buffer> & records() const {
        return records_;
      }

    private:
      guid channel_id_;
      size_t current_term_;
      size_t commit_idx_;
      size_t last_log_idx_;
      std::list<const_data_buffer> records_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_RAFT_APPEND_ENTRIES_H_
