#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "udp_transport.h"
#include "url_parser.h"
#include "db_model.h"
#include "well_known_node_dbo.h"
#include "task_manager.h"
#include "shutdown_event.h"
#include "p2p_crypto_tunnel.h"
#include "udp_socket.h"
#include "p2p_route.h"

namespace vds {

  class _p2p_network : public std::enable_shared_from_this<_p2p_network> {
  public:
    _p2p_network();
    ~_p2p_network();

    async_task<> random_broadcast(
        const vds::service_provider &sp,
        const vds::const_data_buffer &message);

    void add_route(const service_provider &sp, const vds::guid &partner_id,
                       const std::shared_ptr<vds::udp_transport::_session> &session);

    vds::async_task<> init_server(const vds::service_provider &sp, const std::string &user_name,
                                  const std::string &user_password, const std::string &device_name, int port);

    vds::async_task<> start_network(const vds::service_provider &sp);
    async_task<> prepare_to_stop(const vds::service_provider &sp);
    void stop(const vds::service_provider &sp);
  private:
    std::list<class p2p_network_service> network_services_;

    p2p_route route_;

  public:
    leak_detect_helper leak_detect_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_P_H_
