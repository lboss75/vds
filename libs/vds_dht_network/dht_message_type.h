#ifndef __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
#define __VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace network {
      enum class message_type_t {
        Handshake = 0,
        Welcome = 1,

        ContinueData = 2,
        Acknowledgment = 3,

        SpecialCommand = 0b11000000,

        SingleData = 0b01000000,
        Data = 0b10000000,
      };
    }
  }
}
#endif //__VDS_DHT_NETWORK_DHT_MESSAGE_TYPE_H_
