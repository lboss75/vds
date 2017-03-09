#ifndef __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_
#define __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "peer_network_schema.h"
namespace vds {
  class _peer_network_schema
  {
  public:
    _peer_network_schema(peer_network_schema* owner);
    ~_peer_network_schema();

  private:
      peer_network_schema * const owner_;
  };
}

#endif // __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_
