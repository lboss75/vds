#ifndef __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_P_H_
#define __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "consensus_messages.h"

namespace vds {
  namespace consensus_protocol {
    class server;

    class _server
    {
    public:
      _server(
        const service_provider & sp,
        storage_log & storage,
        server * owner,
        certificate & certificate,
        asymmetric_private_key & private_key);

      void start();
      void stop();
      
      void register_server(const std::string & certificate_body);
    private:
      service_provider sp_;
      logger log_;
      storage_log & storage_;
      server * const owner_;
      certificate & certificate_;
      asymmetric_private_key & private_key_;

      std::mutex messages_to_lead_mutex_;
      std::condition_variable messages_to_lead_cond_;
      std::list<std::unique_ptr<json_value>> messages_to_lead_;

      task_job check_leader_task_job_;

      struct node_info
      {

      };

      std::map<std::string, node_info> nodes_;

      enum state
      {
        none,
        leader,
        candidate,
        follower
      };
      state state_;
      size_t leader_check_timer_;

      void leader_check();

      void become_leader();

      void flush_messages_to_lead();
    };
  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_P_H_
