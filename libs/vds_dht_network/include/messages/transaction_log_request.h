#ifndef __VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_
#define __VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_

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
      class transaction_log_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_request;

        transaction_log_request(
            const const_data_buffer &transaction_id,
            const const_data_buffer &source_node)
          : transaction_id_(transaction_id),
            source_node_(source_node) {
        }

        transaction_log_request(
          binary_deserializer & s) {
          s
            >> this->transaction_id_
            >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s
            << this->transaction_id_
            << this->source_node_;
          return s.data();
        }


        const const_data_buffer & transaction_id() const {
          return this->transaction_id_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer transaction_id_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_P2P_NETWORK_COMMON_BLOCK_STATE_H_
