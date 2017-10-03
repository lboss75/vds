#ifndef __VDS_PEER2PEER_PEER2PEER_H_
#define __VDS_PEER2PEER_PEER2PEER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "vds_peer2peer.h"
#include "const_data_buffer.h"

namespace vds {
  
  class p2p_manager
  {
  public:
    
    void connect_to(
      const service_provider & sp,
      const std::string & address);
    
    void  broadcast(
      const service_provider & sp,
      const const_data_buffer & data);
    
    void  send_to(
      const service_provider & sp,
      const p2p_node_id_type & node_id,
      const const_data_buffer & data);
    
  private:
    std::shared_ptr<_p2p_manager> impl_;
	};

}

#endif//__VDS_PEER2PEER_PEER2PEER_H_
