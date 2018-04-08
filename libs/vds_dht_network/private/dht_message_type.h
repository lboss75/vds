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
        channel_log_state = 1,
        offer_move_replica = 2,
        got_replica = 3,
        replica_not_found = 4,
        offer_replica = 5,
        channel_log_request = 6,
        channel_log_record = 7,
        dht_find_node = 8,
        dht_find_node_response = 9,
        dht_ping = 10,
        dht_pong = 11,
      };
    }
  }
}
#endif //__VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
