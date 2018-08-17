#ifndef __VDS_P2P_NETWORK_TRANSACTION_LOG_RECORD_H_
#define __VDS_P2P_NETWORK_TRANSACTION_LOG_RECORD_H_

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
      class transaction_log_record {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_record;

        transaction_log_record(
          const const_data_buffer& record_id,
          const const_data_buffer& data)
          : record_id_(record_id),
            data_(data) {
        }

        transaction_log_record(
          binary_deserializer& s) {
          s >> this->record_id_ >> this->data_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->record_id_ << this->data_;
          return s.move_data();
        }

        const const_data_buffer& record_id() const {
          return record_id_;
        }

        const const_data_buffer& data() const {
          return data_;
        }

      private:
        const_data_buffer record_id_;
        const_data_buffer data_;
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_TRANSACTION_LOG_RECORD_H_
