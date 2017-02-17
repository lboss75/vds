/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_response.h"

vds::http_response::http_response()
  : code_(-1)
{
}

void vds::http_response::reset(
  const vds::http_request& request
)
{
  this->code_ = -1;
  this->headers_.clear();
}

bool vds::http_response::get_header(
  const std::string& name,
  std::string& value)
{
  for(auto& p : this->headers_){
    //Start with
    if(
      p.size() > name.size()
      && p[name.size()] == ':'
      && !p.compare(0, name.size(), name)){
      value = p.substr(name.size() + 1);
      return true;
    }
  }
  
  return false;
}

