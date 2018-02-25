#ifndef __VDS_P2P_NETWORK_CHANNEL_COORDINATOR_P_H_
#define __VDS_P2P_NETWORK_CHANNEL_COORDINATOR_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <set>
#include <list>
#include "guid.h"
#include "node_id_t.h"

namespace vds {
  class _p2p_route;

  namespace p2p_messages {
    class raft_add_client;
    class raft_create_channel;
    class raft_start_election;
  }

  class _channel_coordinator : public std::enable_shared_from_this<_channel_coordinator> {
  public:
    _channel_coordinator(
        const guid & channel_id);

    void start(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route);

    void apply(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route,
        const p2p_messages::raft_create_channel & message);

    void apply(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route,
        const p2p_messages::raft_add_client & message);

    void apply(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route,
        const p2p_messages::raft_request_vote & messsage);

    void apply(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route,
        const p2p_messages::raft_vote_granted & messsage);

    void on_timer(
        const service_provider & sp,
        const std::shared_ptr<_p2p_route> & route);

  private:
    enum class this_member_state_t {
      unknown,
      client,
      follower,
      candidate,
      leader
    };

    struct member_info_t {
      node_id_t id_;
      uint64_t last_log_idx_;
      uint64_t match_idx_;

      bool is_client_;

      member_info_t(const node_id_t & id, bool is_client)
          : id_(id),
            last_log_idx_(0),
            match_idx_(0),
            is_client_(is_client){
      }
    };

    const guid channel_id_;

    std::mutex state_mutex_;
    this_member_state_t state_;

    node_id_t voted_for_;
    node_id_t leader_;

    size_t current_term_;

    size_t last_log_idx_;
    size_t last_log_term_;

    size_t last_applied_idx_;
    size_t commit_idx_;

    int timeout_elapsed_;

    static constexpr int request_timeout_ = 2;
    static constexpr int election_timeout_ = 10;

    int election_timeout_rand_;

    std::map<node_id_t, member_info_t> members_;
    std::set<node_id_t> voted_for_me_;

    enum class record_type_t : uint8_t {
      normal,
      add_client_node,
      add_server_node,
      remove_node
    };

    struct record_info {
      node_id_t source_device;
      uint64_t source_index;
      record_type_t record_type_;
      const_data_buffer data_;
    };
    std::map<size_t, record_info> records_;

    void become_follower(
        const vds::service_provider &sp,
        const std::shared_ptr<vds::_p2p_route> &route);

    void become_leader(
        const vds::service_provider &sp,
        const std::shared_ptr<vds::_p2p_route> &route);

    void become_candidate(
        const vds::service_provider &sp,
        const std::shared_ptr<vds::_p2p_route> &route);

    void send_log(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route,
        const member_info_t &node);

    void apply_entry(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route);

    void election_start(
        const service_provider &sp,
        const std::shared_ptr<_p2p_route> &route);

    size_t majority_count();

    void add_client(const service_provider &sp, const node_id_t &node_id);
  };
}

#endif //__VDS_P2P_NETWORK_CHANNEL_COORDINATOR_P_H_
