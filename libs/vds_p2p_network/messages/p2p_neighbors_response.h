#ifndef __VDS_P2P_NETWORK_P2P_NEIGHBORS_RESPONSE_H_
#define __VDS_P2P_NETWORK_P2P_NEIGHBORS_RESPONSE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "types.h"
#include "guid.h"

namespace vds {
  namespace p2p_messages {
    class p2p_neighbors_response {
    public:

      struct node_info {
        guid node_id_;
        std::string address_;
        std::list<node_info> neighbors_;
      };


    private:
      uint32_t  version_;
      std::list<node_info> neighbors_;
    };

  }
}

#endif //__VDS_P2P_NETWORK_P2P_NEIGHBORS_RESPONSE_H_
