#ifndef __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
#define __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _p2p_network;

  class p2p_network {
  public:

  private:
    std::shared_ptr<_p2p_network> impl_;

  };
}

#endif //__VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
