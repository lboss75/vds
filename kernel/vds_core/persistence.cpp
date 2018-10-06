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
  struct passwd *pw = getpwuid(getuid());
  return foldername(pw->pw_dir);
#else
  CHAR result[MAX_PATH + 1];
  auto error = SHGetFolderPathA(
    NULL,
    CSIDL_COMMON_DOCUMENTS,
    NULL,
    SHGFP_TYPE_CURRENT,
    result);
  if(NO_ERROR != error) {
    throw std::system_error(error, std::system_category(), "SHGetFolderPath");
  }

  for(auto p = strchr(result, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }
  
  return foldername(result);
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

  return foldername(result);
#endif
}
