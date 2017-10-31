#ifndef __VDS_P2P_NETWORK_P2P_TRANSPORT_H_
#define __VDS_P2P_NETWORK_P2P_TRANSPORT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"
#include "async_buffer.h"

namespace vds {

  class p2p_transport {
  public:
    continuous_buffer<const_data_buffer> & incoming();
    continuous_buffer<const_data_buffer> & outgoing();

  private:
    std::shared_ptr<class _p2p_transport> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_TRANSPORT_H_
