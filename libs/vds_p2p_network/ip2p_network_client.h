#ifndef __VDS_P2P_NETWORK_IP2P_NETWORK_CLIENT_H_
#define __VDS_P2P_NETWORK_IP2P_NETWORK_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class ip2p_network_client : public std::enable_shared_from_this<ip2p_network_client> {
  public:
    virtual ~ip2p_network_client(){}

    virtual async_task<> process_message(const guid & user_id, const const_data_buffer & message) = 0;

  };
}

#endif //__VDS_P2P_NETWORK_IP2P_NETWORK_CLIENT_H_
