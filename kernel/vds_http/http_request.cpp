/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_request.h"

bool vds::http_request::get_header(const std::string & name, std::string & value)
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

void vds::http_request::parse_parameters()
{
  this->parameters_.clear();

  auto p = strchr(this->url_.c_str(), '?');
  if (nullptr != p) {
    this->parameters_.push_back(p + 1);
    this->url_.erase(p - this->url_.c_str());
  }
}


