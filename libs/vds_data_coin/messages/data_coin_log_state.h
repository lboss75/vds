#ifndef __VDS_DATA_COIN_COIN_TRANSACTION_H_
#define __VDS_DATA_COIN_COIN_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"

namespace vds {
  namespace data_coin {
    namespace messages {
      class coin_transaction {
      public:
        static const dht::network::message_type_t message_id = dht::network::message_type_t::coin_transaction;

        coin_transaction(
          const std::list<const_data_buffer> & leafs,
          const const_data_buffer & source_node)
          : leafs_(leafs),
          source_node_(source_node) {
        }

        coin_transaction(
          binary_deserializer & s) {
          s >> this->leafs_ >> this->source_node_;
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          s << this->leafs_ << this->source_node_;
          return s.data();
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

#endif //__VDS_DATA_COIN_COIN_TRANSACTION_H_
