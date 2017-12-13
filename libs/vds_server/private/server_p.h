#ifndef __VDS_SERVER_SERVER_P_H_
#define __VDS_SERVER_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server.h"
#include "p2p_network_service.h"

namespace vds {
  class cert_manager;
  class node_manager;
  class user_manager;
  class server_http_api;
  class server_connection;
  class server_udp_api;

  class _node_manager;
  class _chunk_manager;
  class _cert_manager;
  class _local_cache;
  class _server_database;
  class _server_http_api;
  class _storage_log;

  class _server : public iserver
  {
  public:
    _server(server * owner);
    ~_server();
    
    void start(const service_provider &);
    void stop(const service_provider &);

  private:
    friend class server;
    friend class iserver;

    server * owner_;
    certificate certificate_;
    asymmetric_private_key private_key_;

    std::unique_ptr<_node_manager> node_manager_;
    std::unique_ptr<user_manager> user_manager_;
	  std::unique_ptr<class db_model> db_model_;
    std::unique_ptr<_server_http_api> server_http_api_;
    std::unique_ptr<_storage_log> storage_log_;
    std::unique_ptr<_chunk_manager> chunk_manager_;
    std::unique_ptr<_server_database> server_database_;
    std::unique_ptr<_local_cache> local_cache_;

    std::list<p2p_network_service> network_services_;
    std::shared_ptr<class p2p_network_client> network_client_;

    vds::async_task<> init_server(const vds::service_provider &sp, const std::string &user_name,
                                      const std::string &user_password, const std::string &device_name, int port);

    vds::async_task<> start_network(const vds::service_provider &sp);
  };
}

#endif // __VDS_SERVER_SERVER_P_H_
