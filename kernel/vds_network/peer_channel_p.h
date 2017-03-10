#ifndef __VDS_NETWORK_PEER_CHANNEL_P_H_
#define __VDS_NETWORK_PEER_CHANNEL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "peer_channel.h"

namespace vds {
  class _peer_channel
  {
  public:
    _peer_channel();
    virtual ~_peer_channel();

  private:
    friend class peer_channel;

    class peer_channel * owner_;
  };
}

#endif // __VDS_NETWORK_PEER_CHANNEL_P_H_
