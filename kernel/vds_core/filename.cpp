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
  auto p = strrchr(this->value_.c_str(), '/');
  if(nullptr == p){
    return this->value_;
  }
  else {
    return p + 1;
  }
}

std::string vds::filename::extension() const
{
  auto name = this->name();
  auto p = strrchr(name.c_str(), '.');
  if (nullptr == p) {
    return std::string();
  }
  else {
    return p;
  }
}

size_t vds::filename::length() const
{
#ifdef _WIN32
  struct _stat64 stat_buf;
  auto error = _stat64(this->value_.c_str(), &stat_buf);
#else
  struct stat stat_buf;
  auto error = stat(this->value_.c_str(), &stat_buf);
#endif
  if (0 != error) {
    throw new std::system_error(error, std::system_category(), "Failed get file lenght of file " + this->value_);
  }
  return (size_t)stat_buf.st_size;
}