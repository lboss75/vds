#ifndef __VDS_DHT_NETWORK_TRANSACTION_LOG_MESSAGES_H__
#define __VDS_DHT_NETWORK_TRANSACTION_LOG_MESSAGES_H__

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "dht_route_messages.h"

namespace vds {
  namespace dht {
    namespace messages {
      class transaction_log_state {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_state;
        
        std::list<const_data_buffer> leafs;
        const_data_buffer source_node;

        template <typename visitor_type>
        auto visit(visitor_type & v) {
          return v(
            this->leafs,
            this->source_node
          );
        }
      };
      
      class transaction_log_request {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_request;

        const_data_buffer transaction_id;
        const_data_buffer source_node;

        template <typename visitor_type>
        auto visit(visitor_type & v) {
          return v(
            this->transaction_id,
            this->source_node
          );
        }
      };

      class transaction_log_record {
      public:
        static const network::message_type_t message_id = network::message_type_t::transaction_log_record;

        const_data_buffer record_id;
        const_data_buffer data;

        template <typename visitor_type>
        auto visit(visitor_type & v) {
          return v(
            this->record_id,
            this->data
          );
        }
      };
    }
  }
}


#endif//__VDS_DHT_NETWORK_TRANSACTION_LOG_MESSAGES_H__