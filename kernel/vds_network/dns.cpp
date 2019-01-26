/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/dns_p.h"

vds::expected<std::string> vds::_dns::hostname()
{
#ifdef _WIN32
  char result[MAX_PATH + 1];
#else
  char result[255];
#endif
  
  auto error = gethostname(result, sizeof(result));
  if (0 == error) {
    return std::string(result);
  }
#ifdef _WIN32
  error = WSAGetLastError();
#endif
  return vds::make_unexpected<std::system_error>(error, std::system_category(), "Retrieve the standard host name for the local computer failed");
}

vds::_dns_address_info::_dns_address_info()
: first_(nullptr){
}

vds::_dns_address_info::_dns_address_info(addrinfo* first) 
: first_(first){
}

vds::_dns_address_info::~_dns_address_info() {
}

vds::expected<vds::_dns_address_info> vds::_dns_address_info::create(const std::string & hostname)
{
  addrinfo * first;
  auto error = getaddrinfo(hostname.c_str(), NULL, NULL, &first);
  if (0 != error) {
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Retrieve the addresses by the host name " + hostname);
  }

  return _dns_address_info(first);
}
