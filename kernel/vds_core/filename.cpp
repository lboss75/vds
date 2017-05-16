#include "stdafx.h"
#include <string.h>
#include <system_error>
#include "targetver.h"
#include "filename.h"
#include "foldername.h"

vds::filename::filename(
  const vds::foldername& base,
  const std::string& relative)
: value_(base.str() + "/" + relative)
{

}


vds::foldername vds::filename::contains_folder() const
{
  auto p = strrchr(this->value_.c_str(), '/');
  if (nullptr == p) {
    return foldername();
  }
  else {
    return foldername(this->value_.substr(0, p - this->value_.c_str()));
  }
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

//size_t vds::filename::length() const
//{
//#ifdef _WIN32
//  struct _stat64 stat_buf;
//  auto error = _stat64(this->value_.c_str(), &stat_buf);
//#else
//  struct stat stat_buf;
//  auto error = stat(this->value_.c_str(), &stat_buf);
//#endif
//  if (0 != error) {
//    throw std::system_error(error, std::system_category(), "Failed get file lenght of file " + this->value_);
//  }
//  return (size_t)stat_buf.st_size;
//}

vds::filename vds::filename::current_process()
{
#ifdef _WIN32
  char buf[MAX_PATH + 1];
  if (!GetModuleFileName(NULL, buf, MAX_PATH)) {
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Failed get current process ffilename");
  }

  for (auto p = strchr(buf, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }

  return filename(buf);
#else
  char buf[256];
  auto bytes = readlink("/proc/self/exe" /*("/proc/" + std::str(getpid()) + "/exe").c_str()*/, buf, sizeof(buf));
  if (bytes >= 0) {
    buf[bytes] = '\0';
  }
  else {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "Failed get current process ffilename");
  }

  return filename(buf);
#endif
}

