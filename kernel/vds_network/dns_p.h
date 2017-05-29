#ifndef __VDS_NETWORK_DNS_P_H_
#define __VDS_NETWORK_DNS_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "targetver.h"
#include "network_types_p.h"

namespace vds {
  class _dns
  {
  public:
    static std::string hostname();
  };

  class _dns_address_info
  {
  public:
    _dns_address_info(const std::string & hostname);

    addrinfo * first() const
    {
      return this->first_;
    }

  private:
    addrinfo * first_;
  };
}

#endif//__VDS_NETWORK_DNS_P_H_