#ifndef __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
#define __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "consensus_messages.h"

namespace vds {
  class connection_manager;

  namespace consensus_messages {
    class consensus_message_who_is_leader;
  }

  namespace consensus_protocol {

    class _server;
    class server
    {
    public:
      server(
        certificate & certificate,
        asymmetric_private_key & private_key,
        connection_manager & connection_manager);
      ~server();

      void start(const service_provider & sp);
      void stop(const service_provider & sp);

      async_task<const json_value *>
      process(const service_provider & scope,
              const consensus_messages::consensus_message_who_is_leader & message);

    private:
      _server * const impl_;
    };
  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
