#ifndef __VDS_NETWORK_UDP_SOCKET_H_
#define __VDS_NETWORK_UDP_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _udp_socket;
  
  class udp_socket
  {
  public:
    udp_socket();
    ~udp_socket();
    
    
    
  private:
    std::shared_ptr<_udp_socket> impl_;
  };
}

#endif//__VDS_NETWORK_UDP_SOCKET_H_
