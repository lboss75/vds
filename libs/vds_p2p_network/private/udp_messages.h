#ifndef __VDS_P2P_NETWORK_UDP_MESSAGES_H_
#define __VDS_P2P_NETWORK_UDP_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <guid.h>

namespace vds {

/*
   Control:
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |1|type |           Loss Length / ACK Seq. No.                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   ~                 Control Information Field                     ~
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Data
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |0|                         Sequence Number                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/


  static constexpr uint32_t magic_number = (uint32_t)(('V') | ('D' << 8) | ('S' << 16) | ('0' << 24));

  enum class udp_message_id {
    hello_with_login = 0,
    hello_with_login_error = 1,
    hello_with_login_successful_1 = 2,
    hello_with_login_successful_2 = 3,
  };

  //Auth

  struct hello_message {
    guid node_id_;//16 bytes
  };

  struct hello_successful {
    guid node_id_;//16 bytes
    guid from_node_id_;//16 bytes
    uint8_t public_key_[256];//public key
  };

  //Auth by private key
  struct hello_with_cert_message {
    guid node_id_;//16 bytes
  };

  struct hello_with_cert_message {
    guid node_id_;//16 bytes
  };
}

#endif//__VDS_P2P_NETWORK_UDP_MESSAGES_H_
