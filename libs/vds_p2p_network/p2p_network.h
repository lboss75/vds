#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class p2p_network {
  public:
    p2p_network(
        const std::shared_ptr<class ip2p_network_storage> & storage,
        const std::shared_ptr<class ip2p_network_client> & client);

    void login(const std::string & login, const std::string & password);
    void login(const certificate & node_cert, const asymmetric_private_key & node_key);

    async_task<> send_to(
        const service_provider & sp,
        const guid & node_id,
        const const_data_buffer & message);

    async_task<> broadcast(
        const service_provider & sp,
        const const_data_buffer & message);


  private:
    std::shared_ptr<class _p2p_network> impl_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_H_
