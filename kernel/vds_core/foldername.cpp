/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "foldername.h"
#include "filename.h"

vds::foldername::foldername(
  const vds::foldername& base,
  const std::string& relative)
: value_(base.value_ + "/" + relative)
{

}


void vds::foldername::folders(
  const std::function< bool(const foldername &) >& callback
) const
{
#ifdef _WIN32
  WIN32_FIND_DATA ff;
  HANDLE hFind = FindFirstFile((this->value_ + "\\*.*").c_str(), &ff);
  if (hFind == INVALID_HANDLE_VALUE) {
    auto error = GetLastError();
    throw new std::system_error(error, std::system_category(), "get folders of " + this->value_);
  }
  else {
    try {
      do
      {
        if (
          FILE_ATTRIBUTE_DIRECTORY == (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          && nullptr == strstr("..", ff.cFileName)
          ) {
          if (!callback(foldername(*this, ff.cFileName))) {
            break;
          }
        }
      } while (FindNextFile(hFind, &ff));
    }
    catch (...) {
      FindClose(hFind);
      throw;
    }
    FindClose(hFind);
  }
#else
  DIR * d = opendir(this->value_.c_str());
  if (d) {
    try{
      struct dirent *dir;
      while ((dir = readdir(d)) != NULL) {
        if(
          DT_DIR == (dir->d_type & DT_DIR)
          && nullptr == strstr("..", dir->d_name)
        ) {
          if (!callback(foldername(*this, dir->d_name))) {
            break;
          }
        }
      }
    }
    catch(...) {
      closedir(d);
      throw;
    }
    
    closedir(d);
  }
#endif
}

void vds::foldername::files(
  const std::function< bool (const filename &)>& callback) const
{
#ifdef _WIN32
  WIN32_FIND_DATA ff;
  HANDLE hFind = FindFirstFile((this->value_ + "\\*.*").c_str(), &ff);
  if (hFind == INVALID_HANDLE_VALUE) {
    auto error = GetLastError();
    throw new std::system_error(error, std::system_category(), "get folders of " + this->value_);
  }
  else {
    try {
      do
      {
        if (
          FILE_ATTRIBUTE_DIRECTORY != (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          ) {
          if (!callback(filename(*this, ff.cFileName))) {
            break;
          }
        }
      } while (FindNextFile(hFind, &ff));
    }
    catch (...) {
      FindClose(hFind);
      throw;
    }
    FindClose(hFind);
  }
#else
  DIR * d = opendir(this->value_.c_str());
  if (d) {
    try{
      struct dirent *dir;
      while ((dir = readdir(d)) != NULL) {
        if(
          DT_DIR != (dir->d_type & DT_DIR)
        ) {
          if (!callback(filename(*this, dir->d_name))) {
            break;
          }
        }
      }
    }
    catch(...) {
      closedir(d);
      throw;
    }
    
    closedir(d);
  }
#endif
}

std::string vds::foldername::relative_path(const vds::filename & fn, bool allow_pass_border) const
{
  if (fn.value_.length() > this->value_.length()
    && 0 == memcmp(fn.value_.c_str(), this->value_.c_str(), this->value_.length())
    && '/' == fn.value_[this->value_.length()]
    ) {
    return fn.value_.substr(this->value_.length() + 1);
  }

  throw new std::runtime_error("Not implemented");
}

std::string vds::foldername::name() const
{
  auto p = strrchr(this->value_.c_str(), '/');
  if (nullptr == p) {
    return this->value_;
  }
  else {
    return p + 1;
  }
}

void vds::foldername::create()
{
#ifdef _WIN32
  if (0 != _mkdir(this->local_name().c_str())) {
#else
  if (0 != mkdir(this->local_name().c_str(), S_IRWXU | S_IRWXG)) {
#endif//_WIN32
    auto error = errno;
    if (EEXIST != error) {
      throw new std::system_error(error, std::system_category(), "create folder " + this->name());
    }
  }
}