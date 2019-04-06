/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "service_provider.h"

const std::string& vds::service_provider::system_name() {
#ifdef _WIN32
  static bool is_inited = false;
  static std::string result("Win");

  if(!is_inited) {
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    switch (osvi.dwPlatformId) {
    case VER_NT_SERVER:
      result += "Server";
      break;
    }
  }

  return result;
#else
  struct utsname retval;
  if (uname(&retval) < 0) {
  }
#endif

#if defined(__aarch64__)

#elif defined(__ARM__)
#endif

}

int vds::service_provider::system_version() {
  OSVERSIONINFO osvi;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  return osvi.dwMajorVersion * 100 + osvi.dwMinorVersion;
}
