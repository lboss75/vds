#ifndef __VDS_P2P_NETWORK_COMMON_LOG_STATE_H_
#define __VDS_P2P_NETWORK_COMMON_LOG_STATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace dht {
    namespace messages {
      class channel_log_state {
      public:
        static const network::message_type_t message_id = network::message_type_t::channel_log_state;

        channel_log_state(
          const const_data_buffer & channel_id,
          const std::list<const_data_buffer> & leafs,
          const const_data_buffer & source_node)
          : channel_id_(channel_id),
          leafs_(leafs),
          source_node_(source_node) {
        }

        channel_log_state(
          binary_deserializer & s) {
          s >> this->channel_id_ >> this->leafs_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->channel_id_ << this->leafs_ << this->source_node_;
          return s.data();
        }

        const const_data_buffer &channel_id() const {
          return channel_id_;
        }

        const std::list<const_data_buffer> & leafs() const {
          return this->leafs_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer channel_id_;
        std::list<const_data_buffer> leafs_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_LOG_STATE_H_
