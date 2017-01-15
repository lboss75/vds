// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "targetver.h"

#ifndef _WIN32

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Libevent. */
#include <event.h>
#endif

#include <cstring>

#include "network_types.h"
#include "vds_core.h"
#include "vds_network.h"




