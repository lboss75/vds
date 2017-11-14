#ifndef __VDS_P2P_NETWORK_NETWORK_STORAGE_H_
#define __VDS_P2P_NETWORK_NETWORK_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

namespace vds {
  class ip2p_network_storage : public std::enable_shared_from_this<ip2p_network_storage> {
  public:
    virtual ~ip2p_network_storage() {}

  };
}

#endif //__VDS_P2P_NETWORK_NETWORK_STORAGE_H_
