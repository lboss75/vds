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

#include "task_manager.h"
#include "mt_service.h"
#include "network_service.h"
#include "crypto_service.h"
#include "server.h"
#include "web_server.h"
#include "user_manager.h"
#include "db_model.h"
