#ifndef __VDS_DHT_NETWORK_COMMON_LOG_STATE_H_
#define __VDS_DHT_NETWORK_COMMON_LOG_STATE_H_

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
      class transaction_log_state {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_state;

        transaction_log_state(
          const std::list<const_data_buffer> & leafs,
          const const_data_buffer & source_node)
        : leafs_(leafs),
          source_node_(source_node) {
        }

        transaction_log_state(
          binary_deserializer & s) {
          s >> this->leafs_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->leafs_ << this->source_node_;
          return s.move_data();
        }

        const std::list<const_data_buffer> & leafs() const {
          return this->leafs_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        std::list<const_data_buffer> leafs_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_COMMON_LOG_STATE_H_
