#ifndef __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
#define __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "consensus_messages.h"

namespace vds {
  class storage_log;

  namespace consensus_protocol {

    class server_gateway
    {
    public:

      size_t server_count() const;

      void send(const std::list<std::string> & target_ids, const std::string & message);
      void broadcast(const std::string & message);
    };

    class _server;
    class server
    {
    public:
      server(
        const service_provider & sp,
        storage_log & storage,
        certificate & certificate,
        asymmetric_private_key & private_key);
      ~server();

      void start();
      void stop();

      void register_server(const std::string & certificate_body);

    private:
      std::unique_ptr<_server> impl_;
    };
  }
}


#endif // __VDS_PROTOCOLS_CONSENSUS_PROTOCOL_H_
