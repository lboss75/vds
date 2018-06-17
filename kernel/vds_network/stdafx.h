#ifndef __VDS_NETWORK_STDAFX_H_
#define __VDS_NETWORK_STDAFX_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <set>

#include "vds_core.h"
#include "vds_network.h"

#ifdef _WIN32

#include <Ws2tcpip.h>

#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#endif//__VDS_NETWORK_STDAFX_H_

