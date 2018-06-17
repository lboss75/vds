#ifndef __VDS_CORE_STDAFX_H_
#define __VDS_CORE_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "targetver.h"

#include <assert.h>
#include <memory>
#include <string>
#include <typeinfo>
#include <functional>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <memory>
#include <iostream>
#include <set>
#include <list>
#include <string.h>
#include <codecvt>
#include <stdexcept>
#include <system_error>


#ifndef _WIN32
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <locale>
#include <sys/stat.h>

#else
#include <io.h>
#include <direct.h>
#endif//_WIN32

#include <iomanip>


#endif//__VDS_CORE_STDAFX_H_