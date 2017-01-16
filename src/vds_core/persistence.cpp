/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "persistence.h"
#include "foldername.h"

vds::foldername vds::persistence::current_user()
{
#ifndef _WIN32
  return foldername("~/.vds/data");
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
  
  return foldername(result);
#endif
}

vds::foldername vds::persistence::local_machine()
{
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
  
  return foldername(result);
#endif
}
