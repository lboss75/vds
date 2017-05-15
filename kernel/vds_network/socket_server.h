#ifndef __VDS_NETWORK_SOCKET_SERVER_H_
#define __VDS_NETWORK_SOCKET_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "accept_socket_task.h"

namespace vds {
  class service_provider;
  
  class socket_server
  {
  public:
    socket_server(
      const service_provider & sp,
      const std::string & address,
      int port)
    : sp_(sp), address_(address), port_(port)
    {
    }


    template<typename target_class>
    void start(target_class callback)
    {
    }
    
  private:
    const service_provider & sp_;
    std::string address_;
    int port_;
  };
}

#endif // __VDS_NETWORK_SOCKET_SERVER_H_
