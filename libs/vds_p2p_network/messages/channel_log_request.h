#ifndef __VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_
#define __VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class channel_log_request {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::channel_log_request;

      channel_log_request(
          const guid & channel_id,
          const std::list<const_data_buffer> & requests,
          const guid & source_id)
      : channel_id_(channel_id), requests_(requests), source_id_(source_id){
      }

      channel_log_request(
          binary_deserializer & s) {
        s >> this->requests_ >> this->channel_id_ >> this->source_id_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->requests_ << this->channel_id_ << this->source_id_;
        return s.data();
      }

	  const std::list<const_data_buffer> & requests() const {
		  return this->requests_;
	  }

    private:
      guid channel_id_;
      std::list<const_data_buffer> requests_;
      guid source_id_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_
