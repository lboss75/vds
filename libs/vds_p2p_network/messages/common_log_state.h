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
    class common_log_state {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::common_log_state;

      common_log_state(
          const std::list<const_data_buffer> & leafs)
          : leafs_(leafs) {
      }

      common_log_state(
          binary_deserializer & s) {
        s >> this->leafs_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->leafs_;
        return s.data();
      }

    private:
      std::list<const_data_buffer> leafs_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_LOG_STATE_H_
