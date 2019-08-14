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
#include "current_config_dbo.h"
#include "dht_network_client.h"
#include "persistence.h"
#include "user_storage.h"
#include "filename.h"

template<typename result_type>
inline bool check(const vds::expected<result_type> & result)
{
  if (result.has_error()) {
    MessageBoxA(NULL, result.error()->what(), "Виртуальное хранение файлов", MB_ICONERROR);
    return false;
  }

  return true;
}