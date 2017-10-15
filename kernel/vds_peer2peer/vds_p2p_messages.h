#ifndef __VDS_PEER2PEER_P2P_MESSAGES_H_
#define __VDS_PEER2PEER_P2P_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "guid.h"

namespace vds {

  namespace p2p_messages
  {
    class hello_message
    {
    public:
      guid server_id;
      const_data_buffer session_data;
    };
    
    class welcome_message
    {
    public:
      guid server_id;
      const_data_buffer session_data;
    };
    
  };
}

#endif // __VDS_PEER2PEER_P2P_MESSAGES_H_
