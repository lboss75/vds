/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "targetver.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "foldername.h"
#include "filename.h"
#include "file.h"
#include "persistence.h"
#include "../../tests/test_libs/test_config.h"

vds::foldername::foldername(
  const vds::foldername& base,
  const std::string& relative)
: value_(base.value_ + "/" + relative)
{

}


vds::expected<bool> vds::foldername::folders(
  const std::function<expected<bool> (const foldername &) >& callback
) const
{
  bool result = true;
#ifdef _WIN32
  WIN32_FIND_DATA ff;
  HANDLE hFind = FindFirstFile((this->local_name() + "\\*.*").c_str(), &ff);
  if (hFind == INVALID_HANDLE_VALUE) {
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "get folders of " + this->value_);
  }
  else {
    do
    {
      if (
        FILE_ATTRIBUTE_DIRECTORY == (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        && nullptr == strstr("..", ff.cFileName)
        ) {
        auto r = callback(foldername(*this, ff.cFileName));
        if(r.has_error()) {
          FindClose(hFind);
          return unexpected(std::move(r.error()));
        }

        if (!r.value()) {
          result = false;
          break;
        }
      }
    } while (FindNextFile(hFind, &ff));
    FindClose(hFind);
  }
#else
  DIR * d = opendir(this->local_name().c_str());
  if (d) {
    struct dirent *dir;
    struct stat statbuf;
    while ((dir = readdir(d)) != NULL) {
      if(nullptr == strstr("..", dir->d_name)) {
        if (
            stat(foldername(*this, dir->d_name).local_name().c_str(), &statbuf) != -1
            && S_ISDIR(statbuf.st_mode)) {
          auto r = callback(foldername(*this, dir->d_name));
          if(r.has_error()){
            closedir(d);
            return unexpected(std::move(r.error()));
          }

          if (!r.value()) {
            result = false;
            break;
          }
        }
      }
    }
    closedir(d);
  }
#endif
  return expected<bool>(result);
}

vds::expected<bool> vds::foldername::files(
  const std::function<expected<bool> (const filename &)>& callback) const
{
  bool result = true;
#ifdef _WIN32
  WIN32_FIND_DATA ff;
  HANDLE hFind = FindFirstFile((this->local_name() + "\\*.*").c_str(), &ff);
  if (hFind == INVALID_HANDLE_VALUE) {
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "get folders of " + this->value_);
  }
  else {
    do
    {
      if (
        FILE_ATTRIBUTE_DIRECTORY != (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        ) {
        auto r = callback(filename(*this, ff.cFileName));
        if(r.has_error()) {
          FindClose(hFind);
          return unexpected(std::move(r.error()));
        }
        if (!r.value()) {
          result = false;
          break;
        }
      }
    } while (FindNextFile(hFind, &ff));
    FindClose(hFind);
  }
#else
  DIR * d = opendir(this->local_name().c_str());
  if (d) {
    struct dirent *dir;
    struct stat statbuf;
    while ((dir = readdir(d)) != NULL) {
      if(nullptr == strstr("..", dir->d_name)) {
        if (
            stat(filename(*this, dir->d_name).local_name().c_str(), &statbuf) != -1
            && S_ISREG(statbuf.st_mode)) {
          if (!callback(filename(*this, dir->d_name))) {
            result = false;
            break;
          }
        }
      }
    }
    
    closedir(d);
  }
#endif

  return expected<bool>(result);
}

vds::expected<bool> vds::foldername::files_recurcive(
  const std::function<expected<bool>(const filename& name)>& callback) const {
  GET_EXPECTED(result, this->files(callback));
  if(!result) {
    return false;
  }

  return this->folders([callback](const foldername & folder)->expected<bool> {
    return folder.files_recurcive(callback);
  });
}

vds::expected<void> vds::foldername::delete_folder(bool recurcive) const
{
  if (!this->exist()) {
    return expected<void>();
  }

  if (recurcive) {
    CHECK_EXPECTED(this->folders([](const foldername & f) -> expected<bool> {
      CHECK_EXPECTED(f.delete_folder(true));
      return true;
    }));

    CHECK_EXPECTED(this->files([](const filename & f) -> expected<bool> {
      CHECK_EXPECTED(file::delete_file(f));
      return true;
    }));
  }

  if (0 != rmdir(this->local_name().c_str())) {
    auto err = errno;
    return vds::make_unexpected<std::system_error>(err, std::system_category(), "Unable to delete folder " + this->name());
  }

  return expected<void>();
}


vds::expected<std::string> vds::foldername::relative_path(const vds::filename & fn, bool allow_pass_border) const
{
  if (fn.value_.length() > this->value_.length()
    && 0 == memcmp(fn.value_.c_str(), this->value_.c_str(), this->value_.length())
    && '/' == fn.value_[this->value_.length()]
    ) {
    return fn.value_.substr(this->value_.length() + 1);
  }

  return vds::make_unexpected<std::runtime_error>("Not implemented");
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

vds::expected<void> vds::foldername::create() const
{
  auto contains_folder = this->contains_folder();
  if (contains_folder != *this && !contains_folder.exist()) {
    CHECK_EXPECTED(contains_folder.create());
  }

#ifdef _WIN32
  if (0 != _mkdir(this->local_name().c_str())) {
#else
  if (0 != mkdir(this->local_name().c_str(), S_IRWXU | S_IRWXG)) {
#endif//_WIN32
    auto error = errno;
    if (EEXIST != error) {
      return vds::make_unexpected<std::system_error>(error, std::system_category(), "create folder " + this->name());
    }
  }

  return expected<void>();
}

vds::expected<uint64_t> vds::foldername::free_size() const {
#ifndef _WIN32
  struct statvfs buf;
  if(0 != statvfs(this->local_name().c_str(), &buf)){
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Get Free Size");
  }
  return buf.f_bfree * buf.f_frsize;
#else// _WIN32
  ULARGE_INTEGER freeBytesAvailable;
  ULARGE_INTEGER totalNumberOfBytes;
  if(!GetDiskFreeSpaceExA(
    this->local_name().c_str(),
    &freeBytesAvailable,
    &totalNumberOfBytes,
    NULL)){
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Get Free Size");
  }
  return freeBytesAvailable.QuadPart;
#endif// _WIN32
}

vds::expected<uint64_t> vds::foldername::total_size() const {
#ifndef _WIN32
  struct statvfs buf;
  if(0 != statvfs(this->local_name().c_str(), &buf)){
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Get Free Size");
  }
  return buf.f_bsize * buf.f_frsize / (1024 * 1024 * 1024);
#else// _WIN32
  ULARGE_INTEGER freeBytesAvailable;
  ULARGE_INTEGER totalNumberOfBytes;
  if(!GetDiskFreeSpaceExA(
    this->local_name().c_str(),
    &freeBytesAvailable,
    &totalNumberOfBytes,
    NULL)){
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Get Free Size");
  }
  return totalNumberOfBytes.QuadPart;
#endif// _WIN32
}
