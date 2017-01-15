#ifndef __VDS_NETWORK_H_
#define __VDS_NETWORK_H_

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


#include "vds_core.h"

#include "network_manager.h"
#include "input_network_stream.h"
#include "output_network_stream.h"
#include "socket_server.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

#endif // !__VDS_NETWORK_H_

