#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "udp_transport.h"
#include "ip2p_network_storage.h"
#include "url_parser.h"
#include "db_model.h"
#include "well_known_node_dbo.h"
#include "task_manager.h"
#include "shutdown_event.h"
#include "p2p_crypto_tunnel.h"

namespace vds {

  class _p2p_network : public std::enable_shared_from_this<_p2p_network> {
  public:
    _p2p_network(
        const std::shared_ptr<class ip2p_network_storage> &storage,
        const std::shared_ptr<class ip2p_network_client> &client);

    ~_p2p_network();

    void start(
        const class service_provider &sp,
        const std::string &login,
        const std::string &password);

    void start(
        const class service_provider &sp,
        const class certificate &node_cert,
        const class asymmetric_private_key &node_key);

  private:
    std::shared_ptr<class ip2p_network_storage> storage_;
    std::shared_ptr<class ip2p_network_client> client_;

    udp_transport udp_transport_;
    timer backgroud_timer_;

    void start_network(const service_provider &sp);

    bool do_backgroud_tasks(const service_provider &sp);
    void handle_incoming_message(
        const udp_transport::session & source,
        const const_data_buffer & message);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_P_H_
