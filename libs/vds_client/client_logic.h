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
      certificate & client_certificate,
      asymmetric_private_key & client_private_key
    );

    ~client_logic();

    void start();
    void stop();

    void connection_closed(client_connection<client_logic> * connection);
    void connection_error(client_connection<client_logic> * connection, std::exception * ex);

    void process_response(client_connection<client_logic> * connection, const json_value * response);

    void node_install(const std::string & login, const std::string & password);

    std::string get_messages();

  private:
    service_provider sp_;
    logger log_;
    certificate & client_certificate_;
    asymmetric_private_key & client_private_key_;

    std::vector<client_connection<client_logic> *> connection_queue_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;
    
    std::function<void(void)> update_connection_pool_;
    task_job update_connection_pool_task_;

    std::mutex outgoing_queue_mutex_;
    std::list<std::string> outgoing_queue_;
    
    vsr_protocol::iclient vsr_client_;

    install_node_prepare install_node_prepare_message_;

    void update_connection_pool();

    void query_all(const std::string & message);

    void process(client_connection<client_logic>* connection, const install_node_prepared & message);
  };
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
