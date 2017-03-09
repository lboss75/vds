#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class connection_manager;
  
  class _connection_manager
  {
  public:
    _connection_manager(
      const service_provider & sp,
      connection_manager * owner,
      const std::string & from_address);
    ~_connection_manager();

    void start();
    void stop();

    void ready_to_get_messages(iconnection_channel * target);
    void remove_target(iconnection_channel * target);

    void send(const std::list<std::string> & to_address, const std::string &  body);
    void broadcast(const std::string &  body);

  private:
    service_provider sp_;
    connection_manager * const owner_;
    std::string from_address_;
    
    std::mutex connection_channels_mutex_;
    std::list<iconnection_channel *> connection_channels_;
    
    std::mutex messages_mutex_;
    std::list<connection_message> messages_;
    
    void work_thread();

    struct exist_connection
    {
      std::string client_id;
      std::string client_uri;

      std::string server_id;
      std::string server_uri;
    };

    struct direct_connection
    {
      std::string server_id;
      std::string server_uri;
      std::chrono::steady_clock last_usage;
      iconnection_channel * connection;
    };

    std::mutex exist_connections_mutex_;
    std::list<exist_connection> exist_connections_;

    std::mutex direct_connections_mutex_;
    std::list<direct_connection> direct_connections_;

    void connect_by_id(const std::string & server_id);
    void connect_by_uri(const std::string & server_uri);

  };
}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
