#ifndef __VDS_NETWORK_VDS_NETWORK_H_
#define __VDS_NETWORK_VDS_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "targetver.h"

#ifdef _WIN32

#include <Ws2tcpip.h>

#else

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Libevent. */
#include <event.h>
#endif

#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

#endif // __VDS_NETWORK_VDS_NETWORK_H_

