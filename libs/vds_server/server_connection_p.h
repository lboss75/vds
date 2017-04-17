#ifndef __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
#define __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_connection;
  class server_udp_api;

  class _server_connection
  {
  public:
    _server_connection(
      const service_provider & sp,
      server_udp_api * udp_api,
      server_connection * owner);
    ~_server_connection();
    
    void start();
    void stop();



  private:
    service_provider sp_;
    logger log_;
    server_udp_api * udp_api_;
    server_connection * owner_;

    struct endpoint_info
    {
    };

    struct active_connection
    {
    };

    std::map<std::string, active_connection> active_connections_;




    void init_connection(const std::string & address, uint16_t port);
    
    void open_udp_session(const std::string & address);
    void open_https_session(const std::string & address);
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
