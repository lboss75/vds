/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"

bool vds::http_message::get_header(const std::__cxx11::string& name, std::__cxx11::string& value)
{
  for (auto& p : this->headers_) {
    //Start with
    if (
      p.size() > name.size()
      && p[name.size()] == ':'
      && !p.compare(0, name.size(), name)) {
      value = p.substr(name.size() + 1);
      return true;
    }
  }

  return false;
}
