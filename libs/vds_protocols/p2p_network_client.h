#ifndef __VDS_PROTOCOLS_P2P_NETWORK_CLIENT_H_
#define __VDS_PROTOCOLS_P2P_NETWORK_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <async_task.h>
#include <guid.h>
#include "ip2p_network_client.h"

namespace vds {
  class p2p_network_client : public ip2p_network_client {
  public:
    async_task<> process_message(const guid &user_id, const const_data_buffer &message) override;
  };
}

#endif //__VDS_PROTOCOLS_P2P_NETWORK_CLIENT_H_
