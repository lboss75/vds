#ifndef __VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_
#define __VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace p2p_messages {
    enum class p2p_message_id : uint8_t {
      chunk_send_replica = 'r'
    };
  }
}
#endif //__VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_
