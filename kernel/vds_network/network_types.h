#ifndef __NETWORK_TYPES_H_
#define __NETWORK_TYPES_H_

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

#endif//__NETWORK_TYPES_H_
