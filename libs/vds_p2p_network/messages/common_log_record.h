#ifndef __VDS_P2P_NETWORK_COMMON_LOG_RECORD_H_
#define __VDS_P2P_NETWORK_COMMON_LOG_RECORD_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "p2p_message_id.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace p2p_messages {
    class common_log_record {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::common_log_record;

      common_log_record(
          const const_data_buffer & block_id,
          const const_data_buffer & body)
          : block_id_(block_id),
            body_(body) {
      }

      common_log_record(
          binary_deserializer & s) {
        s >> this->block_id_ >> this->body_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->block_id_ << this->body_;
        return s.data();
      }
	  
	  const const_data_buffer & block_id() const {
		  return this->block_id_;
	  }

	  const const_data_buffer & body() const {
		  return this->body_;
	  }

    private:
      const_data_buffer block_id_;
      const_data_buffer body_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_LOG_RECORD_H_
