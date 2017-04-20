#ifndef __VDS_PROTOCOLS_MESSAGE_H_
#define __VDS_PROTOCOLS_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  enum class message_identification : uint32_t
  {
    invalid_message_id = 0,
    hello_message_id = 'h',
    who_is_leader_message_id = 'w',
    leader_candidate_message_id = 'c',

    server_log_record_broadcast_message_id = 'l',
    server_log_get_records_broadcast_message_id = 'r',
  };

  
  class messages
  {
  public:
    
    
  };
}


#endif // __VDS_PROTOCOLS_MESSAGE_H_
