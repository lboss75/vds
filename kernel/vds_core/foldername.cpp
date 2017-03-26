/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "foldername.h"
#include "filename.h"
#include "file.h"

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
  HANDLE hFind = FindFirstFile((this->local_name() + "\\*.*").c_str(), &ff);
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
  DIR * d = opendir(this->local_name().c_str());
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
  HANDLE hFind = FindFirstFile((this->local_name() + "\\*.*").c_str(), &ff);
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
  DIR * d = opendir(this->local_name().c_str());
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

void vds::foldername::delete_folder(bool recurcive) const
{
  if (!this->exist()) {
    return;
  }

  if (recurcive) {
    this->folders([](const foldername & f) -> bool{
      f.delete_folder(true);
      return true;
    });

    this->files([](const filename & f) -> bool {
      file::delete_file(f);
      return true;
    });
  }

  if (0 != rmdir(this->local_name().c_str())) {
    auto err = errno;
    throw new std::system_error(err, std::system_category(), "Unable to delete folder " + this->name());
  }
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

vds::foldername vds::foldername::contains_folder() const
{
  auto p = strrchr(this->value_.c_str(), '/');
  if (nullptr == p) {
    return foldername();
  }
  else {
    return foldername(this->value_.substr(0, p - this->value_.c_str()));
  }
}

bool vds::foldername::exist() const
{
  return (0 == access(this->local_name().c_str(), 0));
}

void vds::foldername::create()
{
  auto contains_folder = this->contains_folder();
  if (contains_folder != *this && !contains_folder.exist()) {
    contains_folder.create();
  }

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