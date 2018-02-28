#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_P_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "p2p_network_service.h"
#include "udp_transport.h"
#include "url_parser.h"
#include "db_model.h"
#include "well_known_node_dbo.h"
#include "task_manager.h"
#include "shutdown_event.h"
#include "p2p_crypto_tunnel.h"
#include "udp_socket.h"
#include "p2p_route.h"
#include "p2p_node_info.h"

namespace vds {
	class node_id_t;
  class _p2p_crypto_tunnel;

  class _p2p_network : public std::enable_shared_from_this<_p2p_network> {
  public:
    _p2p_network();
    ~_p2p_network();

    vds::async_task<> start_network(const vds::service_provider &sp);
    async_task<> prepare_to_stop(const vds::service_provider &sp);
    void stop(const vds::service_provider &sp);

    bool send(
        const vds::service_provider &sp,
        const node_id_t &device_id,
        const vds::const_data_buffer &message);

		bool send_tentatively(
        const service_provider &sp,
        const guid &device_id,
        const const_data_buffer &message,
        size_t distance);

    void process_input_command(
        const service_provider &sp,
        const guid &partner_id,
        const std::shared_ptr<_p2p_crypto_tunnel> &session,
        const vds::const_data_buffer &message);

    void close_session(
        const service_provider &sp,
				const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

	void query_replica(
			const service_provider &sp,
			const const_data_buffer & data_hash);

    void add_node(
        const service_provider &sp,
        const node_id_t & id,
        const std::shared_ptr<_p2p_crypto_tunnel> & proxy_session);

		guid current_node_id() const;
  private:
    p2p_network_service network_service_;
    p2p_route route_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_NETWORK_P_H_
