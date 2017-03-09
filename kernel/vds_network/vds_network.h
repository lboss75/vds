#ifndef __VDS_NETWORK_VDS_NETWORK_H_
#define __VDS_NETWORK_VDS_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "vds_core.h"

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


#include "network_manager.h"
#include "input_network_stream.h"
#include "output_network_stream.h"
#include "socket_server.h"
#include "socket_connect.h"
#include "network_socket.h"
#include "dns.h"
#include "udp_socket.h"
#include "network_serializer.h"
#include "peer_network.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

#endif // __VDS_NETWORK_VDS_NETWORK_H_

