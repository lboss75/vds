#ifndef __VDS_PEER2PEER_VDS_P2P_ROUTE_H_
#define __VDS_PEER2PEER_VDS_P2P_ROUTE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "symmetriccrypto.h"

namespace vds {

  class p2p_route
  {
  public:
    
    
  private:
    struct session_data
    {
      symmetric_key session_key;      
    };
    
    std::map<guid /* server_id */, session_data> items_;
  };
}

#endif // __VDS_PEER2PEER_VDS_P2P_ROUTE_H_
