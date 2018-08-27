#ifndef __VDS_DHT_NETWORK_SYNC_REPLICA_QUERY_OPERATIONS_H_
#define __VDS_DHT_NETWORK_SYNC_REPLICA_QUERY_OPERATIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dht_message_type.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "messages/sync_base_message.h"
#include "sync_message_dbo.h"

namespace vds {
  namespace dht {
    namespace messages {
      class sync_replica_query_operations_request : public sync_base_message_response {
      public:
        static const network::message_type_t message_id = network::message_type_t::sync_replica_query_operations_request;

        sync_replica_query_operations_request(
          const const_data_buffer& object_id,
          uint64_t generation,
          uint64_t current_term,
          uint64_t commit_index,
          uint64_t last_applied)
          : sync_base_message_response(
              object_id,
              generation,
              current_term,
              commit_index,
              last_applied) {
        }

        sync_replica_query_operations_request(
          binary_deserializer& s)
          : sync_base_message_response(s) {
        }

        const_data_buffer serialize() const {
          binary_serializer s;
          sync_base_message_response::serialize(s);
          return s.move_data();
        }
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_SYNC_REPLICA_QUERY_OPERATIONS_H_
