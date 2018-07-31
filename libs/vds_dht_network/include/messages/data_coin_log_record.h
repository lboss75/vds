#ifndef __VDS_DHT_NETWORK_DATA_COIN_LOG_RECORD_H_
#define __VDS_DHT_NETWORK_DATA_COIN_LOG_RECORD_H_

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
      class data_coin_log_record {
      public:
        static const dht::network::message_type_t message_id = dht::network::message_type_t::data_coin_log_record;

        data_coin_log_record(
          const const_data_buffer & transaction_data,
          const const_data_buffer & source_node)
          : transaction_data_(transaction_data),
          source_node_(source_node) {
        }

        data_coin_log_record(
          binary_deserializer & s) {
          s >> this->transaction_data_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->transaction_data_ << this->source_node_;
          return s.data();
        }

        const const_data_buffer & transaction_data() const {
          return this->transaction_data_;
        }

        const const_data_buffer & source_node() const {
          return source_node_;
        }

      private:
        const_data_buffer transaction_data_;
        const_data_buffer source_node_;
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DATA_COIN_LOG_RECORD_H_
