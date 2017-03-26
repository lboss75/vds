#ifndef __VDS_SERVER_SERVER_CONNECTION_H_
#define __VDS_SERVER_SERVER_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server_connection;
  class server_udp_api;
  
  class server_connection
  {
  public:
    server_connection(
      const service_provider & sp,
      server_udp_api * udp_api);
    
    ~server_connection();

    void start();
    void stop();


  private:
    _server_connection * const impl_;
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_H_
