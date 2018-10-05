#ifndef __VDS_NETWORK_SOCKET_TASK_P_H_
#define __VDS_NETWORK_SOCKET_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifdef _WIN32
#include "network_types_p.h"
#include "network_service.h"
#include "private/network_service_p.h"
#include "stream.h"

namespace vds {
  class service_provider;
  
  class _socket_task
  {
  public:
    virtual void process(DWORD dwBytesTransfered) = 0;
    virtual void error(DWORD error_code) = 0;

    static _socket_task * from_overlapped(OVERLAPPED * pOverlapped) {
        return reinterpret_cast<_socket_task *>((uint8_t *)pOverlapped - offsetof(_socket_task, overlapped_));
    }

  protected:
    OVERLAPPED overlapped_;
    WSABUF wsa_buf_;
  };
}

#endif//_WIN32

#endif // __VDS_NETWORK_SOCKET_TASK_P_H_
