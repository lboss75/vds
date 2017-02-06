#ifndef __VDS_NETWORK_DNS_H_
#define __VDS_NETWORK_DNS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class dns
  {
  public:
    static std::string hostname();
  };

  class dns_address_info
  {
  public:
    dns_address_info(const std::string & hostname);

    addrinfo * first() const
    {
      return this->first_;
    }

  private:
    addrinfo * first_;
  };
}

#endif//__VDS_NETWORK_DNS_H_