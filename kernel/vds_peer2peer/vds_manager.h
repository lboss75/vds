#ifndef __VDS_PEER2PEER_PEER2PEER_H_
#define __VDS_PEER2PEER_PEER2PEER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
	class p2p_manager
	{
	public:
		void	broadcast(const const_data_buffer & data);
		void	send_to(const p2p_node_id_type & node_id, const const_data_buffer & data);
	};

}

#endif//__VDS_PEER2PEER_PEER2PEER_H_