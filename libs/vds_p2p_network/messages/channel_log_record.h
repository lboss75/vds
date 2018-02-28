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
    class channel_log_record {
    public:
      static const uint8_t message_id = (uint8_t)p2p_message_id::channel_log_record;

      channel_log_record(
          const guid & channel_id,
          const const_data_buffer & record_id,
          const const_data_buffer & data)
          : channel_id_(channel_id),
            record_id_(record_id),
            data_(data){
      }

      channel_log_record(
          binary_deserializer & s) {
        s >> this->channel_id_ >> this->record_id_ >> this->data_;
      }

      const_data_buffer serialize() const {
        binary_serializer s;
        s << message_id << this->channel_id_ << this->record_id_ << this->data_;
        return s.data();
      }

      const guid & channel_id() const {
        return channel_id_;
      }

      const const_data_buffer & record_id() const {
        return record_id_;
      }

      const const_data_buffer & data() const {
        return data_;
      }

    private:
      guid channel_id_;
      const_data_buffer record_id_;
      const_data_buffer data_;
    };
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_LOG_RECORD_H_
