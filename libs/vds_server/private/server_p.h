#ifndef __VDS_SERVER_SERVER_P_H_
#define __VDS_SERVER_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server.h"

namespace vds {
  namespace transaction_log {
    class sync_process;
  }

  namespace dht {
    namespace network {
      class service;
    }
    namespace messages {
      class transaction_log_state;
      class transaction_log_request;
      class transaction_log_record;
    }
  }

  class cert_manager;
  class user_manager;
  class server_http_api;
  class server_connection;
  class server_udp_api;
  class chunk_manager;

  namespace file_manager {
    class file_manager_service;
  }


  class _server
  {
  public:
    _server(server * owner);
    ~_server();
    
    void start(const service_provider &);
    void stop(const service_provider &);
    async_task<> prepare_to_stop(const service_provider &sp);

    async_task<server_statistic> get_statistic(const vds::service_provider &sp);

    async_task<> apply_message(
      const service_provider & sp,
      database_transaction & t,
      const dht::messages::transaction_log_state & message);

    void apply_message(
      const service_provider& sp,
      database_transaction& t,
      const dht::messages::transaction_log_request& message);

    void apply_message(
      const service_provider& sp,
      database_transaction& t,
      const dht::messages::transaction_log_record & message);

  private:
    friend class server;

    server * owner_;

	  std::unique_ptr<class db_model> db_model_;
    std::unique_ptr<file_manager::file_manager_service> file_manager_;
    std::unique_ptr<dht::network::service> dht_network_service_;

    std::shared_ptr<transaction_log::sync_process> transaction_log_sync_process_;
  };
}

#endif // __VDS_SERVER_SERVER_P_H_
