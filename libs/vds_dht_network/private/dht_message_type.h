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
        got_replica,
        replica_not_found,
        offer_replica,
        channel_log_request,
        dht_find_node,
        dht_find_node_response,
      };

    }
  }
}
#endif //__VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
