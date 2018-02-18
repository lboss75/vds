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
  class user_manager;
  class server_http_api;
  class server_connection;
  class server_udp_api;
  class chunk_manager;

  namespace file_manager {
    class file_manager_service;
  }


  class _server : public iserver
  {
  public:
    _server(server * owner);
    ~_server();
    
    void start(const service_provider &);
    void stop(const service_provider &);
    async_task<> prepare_to_stop(const service_provider &sp);

    async_task<server_statistic> get_statistic(const vds::service_provider &sp);

  private:
    friend class server;
    friend class iserver;

    server * owner_;
    certificate certificate_;
    asymmetric_private_key private_key_;

    std::unique_ptr<user_manager> user_manager_;
	  std::unique_ptr<class db_model> db_model_;
    std::unique_ptr<chunk_manager> chunk_manager_;
    std::unique_ptr<class p2p_network> p2p_network_;
    std::shared_ptr<class p2p_network_client> network_client_;

    std::unique_ptr<class log_sync_service> log_sync_service_;
    std::unique_ptr<file_manager::file_manager_service> file_manager_;
	std::unique_ptr<class chunk_replicator> chunk_replicator_;

  public:
    leak_detect_helper leak_detect_;
  };
}

#endif // __VDS_SERVER_SERVER_P_H_
