#ifndef __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
#define __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace network {
      enum class protocol_message_type_t {
        HandshakeBroadcast = 0,
        Handshake = 1,
        Welcome = 2,

        ContinueData = 3,
        Acknowledgment = 4,

        Failed = 5,

        SpecialCommand = 0b11100000,

        SingleData = 0b00100000,
        Data = 0b01000000,

        RouteSingleData = 0b01100000,
        RouteData = 0b10000000,

        ProxySingleData = 0b10100000,
        ProxyData = 0b11000000,
      };

      enum class message_type_t {
        transaction_log_state,
        transaction_log_request,
        transaction_log_record,
        //got_replica,
        //replica_request,
        //replica_not_found,
        //offer_replica,
        //sync_replica_data,
        dht_find_node,
        dht_find_node_response,
        dht_ping,
        dht_pong,

        sync_new_election_request,
        sync_new_election_response,

        sync_add_message_request,

        sync_leader_broadcast_request,
        sync_leader_broadcast_response,

        sync_replica_operations_request,
        sync_replica_operations_response,

        sync_looking_storage_request,
        sync_looking_storage_response,

        sync_snapshot_request,
        sync_snapshot_response,

        sync_offer_send_replica_operation_request,
        sync_offer_remove_replica_operation_request,
        //sync_offer_replica_operation_response,

        sync_replica_request,
        sync_replica_data

      };
    }
  }
}

namespace std {

  inline std::string to_string(const vds::dht::network::message_type_t message_type) {
    switch (message_type) {

#define out_member(member)\
    case vds::dht::network::message_type_t::member:\
      return #member;

      out_member(transaction_log_state)
        out_member(transaction_log_request)
        out_member(transaction_log_record)
        //out_member(got_replica)
        //out_member(replica_request)
        //out_member(replica_not_found)
        //out_member(offer_replica)
        //out_member(sync_replica_data)
        out_member(dht_find_node)
        out_member(dht_find_node_response)
        out_member(dht_ping)
        out_member(dht_pong)

        out_member(sync_new_election_request)
        out_member(sync_new_election_response)

        out_member(sync_add_message_request)

        out_member(sync_leader_broadcast_request)
        out_member(sync_leader_broadcast_response)

        out_member(sync_replica_operations_request)
        out_member(sync_replica_operations_response)

        out_member(sync_looking_storage_request)
        out_member(sync_looking_storage_response)

        out_member(sync_snapshot_request)
        out_member(sync_snapshot_response)

        out_member(sync_offer_send_replica_operation_request)
        out_member(sync_offer_remove_replica_operation_request)
        //out_member(sync_offer_replica_operation_response)

        out_member(sync_replica_request)
        out_member(sync_replica_data)
    }
    return "unknown";
    }
}
#endif //__VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
