#ifndef __VDS_WEB_SERVER_STDAFX_H_
#define __VDS_WEB_SERVER_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifndef _WIN32
#include <unistd.h>
#include <limits.h>
#include <sys/statvfs.h>
#endif

#include "vds_core.h"
#include "vds_data.h"
#include "vds_database.h"
#include "vds_http.h"
#include "vds_parser.h"
#include "vds_crypto.h"

#define ThisModule "WebServer"

#endif // __VDS_WEB_SERVER_STDAFX_H_
