#ifndef __VDS_SERVER_SERVER_P_H_
#define __VDS_SERVER_SERVER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server.h"
#include "debug_mutex.h"
#include "imessage_map.h"

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
      : public std::enable_shared_from_this<_server>,
        public dht::network::imessage_map
  {
  public:
    _server(server * owner);
    ~_server();
    
    void start(const service_provider &);
    void stop(const service_provider &);
    vds::async_task<void> prepare_to_stop(const service_provider &sp);

    vds::async_task<server_statistic> get_statistic(const vds::service_provider &sp);

    vds::async_task<void> process_message(
        const service_provider& scope,
        const message_info_t & message_info) override;


    async_task<void> on_new_session(
      const service_provider& sp,
      const const_data_buffer& partner_id) override;

      vds::async_task<void> apply_message(
      const service_provider & sp,
      database_transaction & t,
      const dht::messages::transaction_log_state & message,
      const message_info_t & message_info);

    void apply_message(
      const service_provider& sp,
      database_transaction& t,
      const dht::messages::transaction_log_request& message,
      const message_info_t & message_info);

    void apply_message(
      const service_provider& sp,
      database_transaction& t,
      const dht::messages::transaction_log_record & message,
      const message_info_t & message_info);

  private:
    friend class server;

    server * owner_;

    timer update_timer_;
    std::debug_mutex update_timer_mutex_;
    bool in_update_timer_ = false;

	  std::unique_ptr<class db_model> db_model_;
    std::unique_ptr<file_manager::file_manager_service> file_manager_;
    std::shared_ptr<dht::network::iudp_transport> udp_transport_;
    std::unique_ptr<dht::network::service> dht_network_service_;

    std::shared_ptr<transaction_log::sync_process> transaction_log_sync_process_;
  };
}

#endif // __VDS_SERVER_SERVER_P_H_
