#ifndef __VDS_P2P_NETWORK_COMMON_LOG_STATE_H_
#define __VDS_P2P_NETWORK_COMMON_LOG_STATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class channel_log_state {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::channel_log_state;

      channel_log_state(
          const guid & channel_id,
          const std::list<const_data_buffer> & leafs,
          const guid & source_node)
          : channel_id_(channel_id),
            leafs_(leafs),
            source_node_(source_node) {
      }

      channel_log_state(
          binary_deserializer & s) {
        s >> this->channel_id_ >>  this->leafs_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->channel_id_ << this->leafs_;
        return s.data();
      }

      const guid &channel_id() const {
        return channel_id_;
      }

      const std::list<const_data_buffer> & leafs() const {
        return this->leafs_;
      }

      const guid & source_node() const {
        return source_node_;
      }

    private:
      guid channel_id_;
      std::list<const_data_buffer> leafs_;
      guid source_node_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_LOG_STATE_H_
