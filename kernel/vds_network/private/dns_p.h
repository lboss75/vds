#ifndef __VDS_NETWORK_DNS_P_H_
#define __VDS_NETWORK_DNS_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "targetver.h"
#include "network_types_p.h"

namespace vds {
  class _dns
  {
  public:
    static expected<std::string> hostname();
  };

  class _dns_address_info
  {
  public:
    _dns_address_info();
    explicit _dns_address_info(addrinfo * first);
    ~_dns_address_info();

    static expected<_dns_address_info> create(const std::string & hostname);

    addrinfo * first() const
    {
      return this->first_;
    }

  private:
    addrinfo * first_;
  };
}

#endif//__VDS_NETWORK_DNS_P_H_