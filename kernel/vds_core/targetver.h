#ifndef __VDS_CORE_TARGETVER_H_
#define __VDS_CORE_TARGETVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <SDKDDKVer.h>
#include <windows.h>
#include <Shlobj.h>
#include <io.h>

#pragma comment(lib, "Shell32.lib")

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#else

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
 
#endif

#include "stddef.h"

//#include <vector>
//#include <list>
//#include <unordered_map>
//#include <thread>
//#include <future>
//#include <chrono>
//#include <queue>
//#include <algorithm>
//#include <stack>
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <fcntl.h>

#endif// __VDS_CORE_TARGETVER_H_