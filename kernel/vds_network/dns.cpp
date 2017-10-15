/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/dns_p.h"

std::string vds::_dns::hostname()
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
  throw std::system_error(error, std::system_category(), "Retrieve the standard host name for the local computer failed");
}

vds::_dns_address_info::_dns_address_info(const std::string & hostname)
{
  auto error = getaddrinfo(hostname.c_str(), NULL, NULL, &this->first_);
  if (0 != error) {
    throw std::system_error(error, std::system_category(), "Retrieve the addresses by the host name " + hostname);
  }
}
