#ifndef __VDS_NETWORK_UDP_DATAGRAM_SIZE_EXCEPTION_H_
#define __VDS_NETWORK_UDP_DATAGRAM_SIZE_EXCEPTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdexcept>

namespace vds {

  class udp_datagram_size_exception : public std::runtime_error {
  public:
    udp_datagram_size_exception();

  };
}


#endif //__VDS_NETWORK_UDP_DATAGRAM_SIZE_EXCEPTION_H_
