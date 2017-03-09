#ifndef __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
#define __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_connection;

  class _server_connection : private iconnection_channel
  {
  public:
    _server_connection(
      const service_provider & sp,
      server_connection * owner);
    ~_server_connection();

    void get_delivery_metrics(
      std::map<std::string, size_t> & metrics) override;
      
    void send(
      const std::string & from_address,
      std::list<std::string> & to_address,
      const std::string &  body) override;

  private:
    service_provider sp_;
    server_connection * owner_;

    struct endpoint_info
    {
    };

    struct active_connection
    {
    };

    std::map<std::string, active_connection> active_connections_;




    void init_connection(const std::string & address, uint16_t port);
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
