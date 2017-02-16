#ifndef __VDS_PARSER_STDAFX_H_
#define __VDS_PARSER_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifndef _WIN32
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif//_WIN32


#include <exception>
#include <string>
#include <sstream>
#include <iomanip>

#include "vds_core.h"

#endif//__VDS_PARSER_STDAFX_H_
