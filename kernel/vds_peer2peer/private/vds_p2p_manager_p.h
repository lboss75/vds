#ifndef __VDS_PEER2PEER_PEER2PEER_P_H_
#define __VDS_PEER2PEER_PEER2PEER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
	class _p2p_manager
	{
	public:
    void connect_to(
      const service_provider & sp,
      const std::string & address)
    {
      
    }

		void	broadcast(
      const const_data_buffer & data);
    
		void	send_to(
      const p2p_node_id_type & node_id,
      const const_data_buffer & data);
    
  private:
	};

}

#endif//__VDS_PEER2PEER_PEER2PEER_P_H_
