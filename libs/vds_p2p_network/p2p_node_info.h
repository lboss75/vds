#ifndef __VDS_P2P_NETWORK_P2P_NODE_INFO_H_
#define __VDS_P2P_NETWORK_P2P_NODE_INFO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "guid.h"

namespace vds {
  namespace p2p {

    struct p2p_node_info {
      guid node_id;

      bool operator < (const p2p_node_info & other) const {
        return this->node_id < other.node_id;
      }

      bool operator == (const p2p_node_info & other) const {
        return this->node_id == other.node_id;
      }
    };
  }
}

#endif //__VDS_P2P_NETWORK_P2P_NODE_INFO_H_
