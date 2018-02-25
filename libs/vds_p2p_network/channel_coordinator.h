#ifndef __VDS_P2P_NETWORK_CHANNEL_COORDINATOR_H_
#define __VDS_P2P_NETWORK_CHANNEL_COORDINATOR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <messages/raft_create_channel.h>

namespace vds {
  class _p2p_route;

  class _channel_coordinator;

  namespace p2p_messages {
    class raft_add_client;
  }

  class channel_coordinator : public std::enable_shared_from_this<channel_coordinator> {
  public:

    void apply(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route,
        const p2p_messages::raft_add_client &messsage);

    void apply(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route,
        const p2p_messages::raft_create_channel &messsage);

    void start(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route);

    void stop(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route);

  private:
    std::shared_mutex coordinators_mutex_;
    std::map<guid, std::shared_ptr<_channel_coordinator>> coordinators_;
    timer update_timer_;

    std::shared_ptr<_channel_coordinator> get_or_create_coordinator(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route,
        const guid &channel_id);
  };

}

#endif //__VDS_P2P_NETWORK_CHANNEL_COORDINATOR_H_
