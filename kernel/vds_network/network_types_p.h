#ifndef __NETWORK_TYPES_P_H_
#define __NETWORK_TYPES_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>

/* Libevent. */
#include <event.h>

#endif

#ifdef _WIN32
typedef SOCKET SOCKET_HANDLE;
typedef int socklen_t;
#else
typedef int SOCKET_HANDLE;
#define INVALID_SOCKET (-1)
#define SD_BOTH 2
#endif


#endif//__NETWORK_TYPES_P_H_
