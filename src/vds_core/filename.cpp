#include "stdafx.h"
#include "filename.h"
#include "foldername.h"

vds::filename::filename(
  const vds::foldername& base,
  const std::string& relative)
: value_(base.str() + "/" + relative)
{

}


std::string vds::filename::name() const
{
#ifndef _WIN32
  auto p = strrchr(this->value_.c_str(), '/');
  if(nullptr == p){
    return this->value_;
  }
  else {
    return p + 1;
  }
#else
  auto p = strrchr(this->value_.c_str(), '/');
  if(nullptr == p){
    return this->value_;
  }
  else {
    return p + 1;
  }
#endif//_WIN32
}
