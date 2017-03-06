#ifndef __VDS_SERVER_SERVER_UDP_API_P_H_
#define __VDS_SERVER_SERVER_UDP_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_udp_api;
  class _server_udp_api
  {
  public:
    _server_udp_api(const service_provider & sp, server_udp_api * owner);
    ~_server_udp_api();
      
    void start(const std::string & address, size_t port);
    void stop();

  private:
    service_provider sp_;
    server_udp_api * const owner_;
    udp_socket s_;
  };
}

#endif // __VDS_SERVER_SERVER_UDP_API_P_H_

