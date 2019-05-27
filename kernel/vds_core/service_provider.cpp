/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "service_provider.h"

#ifndef _WIN32
#include <sys/utsname.h>
#endif

std::string vds::service_provider::system_name() {
#ifdef _WIN32
  std::string result("Win");

  OSVERSIONINFO osvi;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  switch (osvi.dwPlatformId) {
  case VER_NT_SERVER:
    result += "Server";
    break;
  }

  return result;
#else
  struct utsname retval;
  if (uname(&retval) < 0) {
    return "Linux";
  }
  else {
    return retval.sysname;
  }
#endif

}

vds::version vds::service_provider::system_version() {
#ifdef _WIN32
  OSVERSIONINFO osvi;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  return version(osvi.dwMajorVersion, osvi.dwMinorVersion);
#else
  struct utsname retval;
  if (uname(&retval) < 0) {
    return version(0, 0);
  }
  else {
    return version::parse(retval.version);
  }

#endif
}
