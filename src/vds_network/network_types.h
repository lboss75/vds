#ifndef __NETWORK_TYPES_H_
#define __NETWORK_TYPES_H_

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#endif

#endif//__NETWORK_TYPES_H_
