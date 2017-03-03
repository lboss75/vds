#ifndef __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
#define __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "consensus_messages.h"

namespace vds {
  class storage_log;

  namespace consensus_messages {
    class consensus_message_who_is_leader;
  }

  namespace consensus_protocol {

    class iserver_gateway
    {
    public:
      virtual void send(const std::list<std::string> & target_ids, const std::string & message) = 0;
      virtual void broadcast(const std::string & message) = 0;
    };

    class _server;
    class server
    {
    public:
      server(
        const service_provider & sp,
        certificate & certificate,
        asymmetric_private_key & private_key,
        iserver_gateway & server_gateway);
      ~server();

      void start();
      void stop();

      void register_server(const std::string & certificate_body);

      void process(const service_provider & scope, json_array * result, const consensus_messages::consensus_message_who_is_leader & message);

    private:
      std::unique_ptr<_server> impl_;
    };
  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
