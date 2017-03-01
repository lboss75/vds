/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "url_parser.h"

bool vds::url_parser::parse_addresses(
  const std::string & addresses,
  const std::function<bool(const std::string& protocol, const std::string& address)> & handler)
{
  std::string::size_type start = 0;
  auto p = addresses.find(';', start);
  while (std::string::npos != p) {
    auto item = addresses.substr(start, p - start);
    if (!parse_address(item, handler)) {
      return false;
    }
  }

  return parse_address(addresses.substr(start), handler);
}

bool vds::url_parser::parse_address(
  const std::string & address,
  const std::function<bool(const std::string&protocol, const std::string&address)> & handler)
{
  auto p = address.find(':');
  if (std::string::npos == p) {
    return false;
  }

  return handler(address.substr(0, p), address);
}

vds::url_parser::http_address vds::url_parser::parse_http_address(const std::string & address)
{
  vds::url_parser::http_address result;

  auto p = address.find(':');
  if (std::string::npos == p
    || p + 2 > address.length()
    || '/' != address[p + 1]
    || '/' != address[p + 2]) {
    return vds::url_parser::http_address();
  }

  result.protocol = address.substr(0, p);
  auto start = p + 3;

  p = address.find('/', start);
  if (std::string::npos == p) {
    auto p1 = address.find(':', start);
    if (std::string::npos == p1) {
      result.server = address.substr(start);
    }
    else {
      result.server = address.substr(start, p1 - start);
      result.port = address.substr(p1 + 1);
    }
  }
  else {
    auto p1 = address.find(':', start);
    if (std::string::npos == p1) {
      result.server = address.substr(start, p - start);
    }
    else if (p < p1) {
      result.server = address.substr(start, p - start);
    }
    else {
      result.server = address.substr(start, p1 - start);
      result.port = address.substr(p1 + 1, p - p1 - 1);
    }

    result.path = address.substr(p);
  }

  return result;
}
