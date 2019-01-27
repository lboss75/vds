#ifndef __VDS_NETWORK_SOCKET_BASE_H_
#define __VDS_NETWORK_SOCKET_BASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef _WIN32

namespace vds {
  class service_provider;
  
  class socket_base : public std::enable_shared_from_this<socket_base>
  {
  public:
    virtual ~socket_base() {}
    virtual expected<void> process(uint32_t events) = 0;
  };
}

#endif//_WIN32

#endif // __VDS_NETWORK_SOCKET_BASE_H_
