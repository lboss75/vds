/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "persistence.h"
#include "foldername.h"

vds::foldername vds::persistence::current_user(const service_provider & sp)
{
  if (!sp.current_user_folder_.empty()) {
    return sp.current_user_folder_;
  }

#ifndef _WIN32
  struct passwd *pw = getpwuid(getuid());
  return foldername(pw->pw_dir);
#else
  CHAR result[MAX_PATH + 1];
  auto error = SHGetFolderPathA(
    NULL,
    CSIDL_PERSONAL,
    NULL,
    SHGFP_TYPE_CURRENT,
    result);
  if(NO_ERROR != error) {
    throw new std::system_error(error, std::system_category(), "SHGetFolderPath");
  }

  for(auto p = strchr(result, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }
  
  return foldername(result);
#endif
}

vds::foldername vds::persistence::local_machine(const service_provider & sp)
{
  if (!sp.local_machine_folder_.empty()) {
    return sp.local_machine_folder_;
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
    throw new std::system_error(error, std::system_category(), "SHGetFolderPath");
  }

  for (auto p = strchr(result, '\\'); nullptr != p; p = strchr(p + 1, '\\')) {
    *p = '/';
  }

  return foldername(result);
#endif
}
