#ifndef __VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_
#define __VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"

namespace vds {
  namespace p2p_messages {
    enum class p2p_message_id : uint8_t {
      //Save log
      channel_log_record = 'l',
      channel_log_state = 's',
      channel_log_request = 'r',

      chunk_query_replica = 'q',
      chunk_have_replica = 'h',

      chunk_send_replica = 'c',
      chunk_offer_replica = 'o',

      dht_ping = 'p',
      dht_find_node = 'f',
      dht_find_node_response = 'a',

      raft_get_lead = 'S',
      raft_current_lead = 'L',
      raft_start_election = 'E',
      raft_vote_granted = 'G',
      raft_request_vote = 'R',
      raft_append_entries = 'A'

    };
  }
}
#endif //__VDS_P2P_NETWORK_P2P_MESSAGE_ID_H_
