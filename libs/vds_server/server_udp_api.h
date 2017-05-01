#ifndef __VDS_SERVER_SERVER_UDP_API_H_
#define __VDS_SERVER_SERVER_UDP_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server_udp_api;
  
  class server_udp_api
  {
  public:
    server_udp_api();
    
    ~server_udp_api();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);
    
  private:
      _server_udp_api * const impl_;
  };
}
#endif // __VDS_SERVER_SERVER_UDP_API_H_
