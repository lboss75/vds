#ifndef __VDS_PEER2PEER_P2P_MESSAGES_H_
#define __VDS_PEER2PEER_P2P_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  namespace p2p_messages
  {
    class hello_message
    {
    public:
      guid server_id;
      std::string address;
    };
    
    class hello_message
    {
    public:
      guid server_id;
      std::string address;
      const_data_buffer key;
    };
    
  };
}

#endif // __VDS_PEER2PEER_P2P_MESSAGES_H_
