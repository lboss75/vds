#ifndef __VDS_SERVER_SERVER_P_H_
#define __VDS_SERVER_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server.h"

namespace vds {
  class cert_manager;
  class node_manager;
  class user_manager;
  class server_http_api;
  class storage_log;
  class server_connection;
  class server_udp_api;

  class _node_manager;

  class _server
  {
  public:
    _server(server * owner);
    ~_server();
    
    void start(const service_provider &);
    void stop(const service_provider &);
    
    void set_port(size_t port);
    
  private:
    friend class server;
    friend class iserver;

    server * owner_;
    certificate certificate_;
    asymmetric_private_key private_key_;
    size_t port_;

    std::unique_ptr<server_connection> server_connection_;

    std::unique_ptr<consensus_protocol::server> consensus_server_protocol_;
    std::unique_ptr<cert_manager> cert_manager_;
    std::unique_ptr<_node_manager> node_manager_;
    std::unique_ptr<user_manager> user_manager_;
    
    std::unique_ptr<server_http_api> server_http_api_;
    std::unique_ptr<server_udp_api> server_udp_api_;
    std::unique_ptr<connection_manager> connection_manager_;
    std::unique_ptr<peer_network> peer_network_;
    
    std::unique_ptr<storage_log> storage_log_;
    std::unique_ptr<chunk_manager> chunk_manager_;
    std::unique_ptr<server_database> server_database_;
    std::unique_ptr<local_cache> local_cache_;
  };
}

#endif // __VDS_SERVER_SERVER_P_H_
