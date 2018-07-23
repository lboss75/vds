#ifndef __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
#define __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace network {
      enum class protocol_message_type_t{
        HandshakeBroadcast = 0,
        Handshake = 1,
        Welcome = 2,

        ContinueData = 3,
        Acknowledgment = 4,
        
        Failed = 5,

        SpecialCommand = 0b11000000,

        SingleData = 0b01000000,
        Data = 0b10000000,
      };

      enum class message_type_t {
        transaction_log_state,
        got_replica,
        replica_request,
        replica_not_found,
        offer_replica,
        replica_data,
        transaction_log_request,
        transaction_log_record,
        dht_find_node,
        dht_find_node_response,
        dht_ping,
        dht_pong,

        sync_new_election_request,
        sync_new_election_response,

        sync_coronation_request,
        sync_coronation_response,

        sync_apply_operation_request,
        sync_apply_operation_response,

        sync_leader_broadcast_request,
        sync_leader_broadcast_response,

        sync_replica_operations_request,
        sync_replica_operations_response,

        sync_looking_storage_request,
        sync_looking_storage_response,
      };
    }
  }
}
#endif //__VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
