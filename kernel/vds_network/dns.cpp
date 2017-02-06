/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "dns.h"

std::string vds::dns::hostname()
{
  char result[MAX_PATH + 1];
  if (0 == gethostname(result, MAX_PATH)) {
    return std::string(result);
  }

  auto error = WSAGetLastError();
  throw new std::system_error(error, std::system_category(), "Retrieve the standard host name for the local computer failed");
}

vds::dns_address_info::dns_address_info(const std::string & hostname)
{
  auto error = getaddrinfo(hostname.c_str(), NULL, NULL, &this->first_);
  if (0 != error) {
    throw new std::system_error(error, std::system_category(), "Retrieve the addresses by the host name " + hostname);
  }
}
