#ifndef __VDS_NETWORK_SOCKET_TASK_P_H_
#define __VDS_NETWORK_SOCKET_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "network_types_p.h"

namespace vds {
  
  class _socket_task
  {
  public:
    _socket_task();
    virtual ~_socket_task();
    
#ifdef _WIN32
    virtual void process(DWORD dwBytesTransfered) = 0;
    virtual void error(DWORD error_code) = 0;

    static _socket_task * from_overlapped(OVERLAPPED * pOverlapped) {
        return reinterpret_cast<_socket_task *>((uint8_t *)pOverlapped - offsetof(_socket_task, overlapped_));
    }
#endif//_WIN32

  protected:
#ifdef _WIN32
    OVERLAPPED overlapped_;
    WSABUF wsa_buf_;
#endif//_WIN32
  };
  
#ifndef _WIN32
  class _socket_handler
  {
  public:
    virtual void process(uint32_t events) = 0;
  };
  
#endif
}

#endif // __VDS_NETWORK_SOCKET_TASK_P_H_
