#ifndef __STDAFX_H_
#define __STDAFX_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifndef _WIN32
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif//_WIN32

#include <memory>
#include <exception>
#include <string>
#include <sstream>
#include <list>
#include <map>
#include <fstream>
#include <iostream>
#include <functional>

#include "vds_core.h"
#include "vds_parser.h"
#include "vruntime.h"

#endif//__STDAFX_H_
