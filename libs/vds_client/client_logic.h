#ifndef __VDS_CLIENT_CLIENT_LOGIC_H_
#define __VDS_CLIENT_CLIENT_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"

namespace vds {
  class client_logic
  {
  public:
    client_logic(
      const service_provider & sp,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    );

    ~client_logic();

    void start();
    void stop();

    void connected(client_connection<client_logic> & connection);
    void connection_closed(client_connection<client_logic> & connection);
    void connection_error(client_connection<client_logic> & connection, std::exception * ex);

    void process_response(client_connection<client_logic> & connection, const json_value * response);

    void node_install(const std::string & login, const std::string & password);

    void get_commands(client_connection<client_logic> & connection);

  private:
    service_provider sp_;
    logger log_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;

    std::vector<std::unique_ptr<client_connection<client_logic>>> connection_queue_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;
    
    std::function<void(void)> update_connection_pool_;
    task_job update_connection_pool_task_;

    pipeline_queue<std::string, client_connection<client_logic>> outgoing_queue_;
    
    void update_connection_pool();
  };
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
