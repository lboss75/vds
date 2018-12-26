/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <system_error>
#include "persistence.h"
#include "foldername.h"

#ifndef WIN32
#include <pwd.h>
#endif

vds::foldername vds::persistence::current_user(const service_provider * sp)
{
  auto props = sp->current_user();
  if (!props.empty()) {
    return props;
  }

#ifndef _WIN32
  auto bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1) {
    bufsize = 0x4000; // = all zeroes with the 14th bit set (1 << 14)
  }

  auto buf = malloc(bufsize);
  if (buf == NULL) {
    throw std::runtime_error("Out of memory");
  }

  struct passwd pwd;
  struct passwd *result;
  auto s = getpwuid_r(getuid(), &pwd, (char *)buf, bufsize, &result);
  if (result == NULL) {
    free(buf);
    if (s == 0) {
      throw std::runtime_error("Not found user folder");
    } else {
      throw std::system_error(s, std::system_category(), "getpwnam_r");
    }
  }

  foldername r(result->pw_dir);
  free(buf);
  return r;

  //struct passwd *pw = getpwuid(getuid());
  //return foldername(pw->pw_dir);
#else
  CHAR result[MAX_PATH + 1];
  auto error = SHGetFolderPathA(
    NULL,
    CSIDL_PERSONAL,
    NULL,
    SHGFP_TYPE_CURRENT,
    result);
  if(NO_ERROR != error) {
    throw std::system_error(error, std::system_category(), "SHGetFolderPath");
  }

  for(auto p = strchr(result, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }
  
  return foldername(foldername(result), ".vds");
#endif
}

vds::foldername vds::persistence::local_machine(const service_provider * sp)
{
  auto props = sp->local_machine();
  if (!props.empty()) {
    return props;
  }

#ifndef _WIN32
  return foldername("/etc/vds/data");
#else
  CHAR result[MAX_PATH + 1];
  auto error = SHGetFolderPathA(
    NULL,
    CSIDL_LOCAL_APPDATA,
    NULL,
    SHGFP_TYPE_CURRENT,
    result);
  if(NO_ERROR != error) {
    throw std::system_error(error, std::system_category(), "SHGetFolderPath");
  }

  for (auto p = strchr(result, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }

  return foldername(foldername(result), ".vds");
#endif
}
