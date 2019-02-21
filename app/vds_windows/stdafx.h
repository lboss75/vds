// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <shellapi.h>
#include <commctrl.h>

#include <tchar.h>

#include "resource.h"

#include <string>

namespace std {
#ifdef _UNICODE
  typedef wstring tstring;
#else
  typedef string tstring;
#endif
}

#include "../vds_embedded/vds_api.h"