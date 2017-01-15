#ifndef __VDS_NETWORK_SOCKET_TASK_H_
#define __VDS_NETWORK_SOCKET_TASK_H_

#include "network_socket.h"

namespace vds {
  
  class socket_task
  {
  public:
#ifdef _WIN32
      virtual ~socket_task();

      virtual void process(DWORD dwBytesTransfered) = 0;

      static socket_task * from_overlapped(OVERLAPPED * pOverlapped) {
          return reinterpret_cast<socket_task *>((uint8_t *)pOverlapped - offsetof(socket_task, overlapped_));
      }
#else//!_WIN32
#endif//_WIN32

  protected:
    network_socket::SOCKET_HANDLE s_;
    
#ifdef _WIN32
      OVERLAPPED overlapped_;
      WSABUF wsa_buf_;
#else//!_WIN32
      struct event event_;
#endif//_WIN32
  };
}

#endif // __VDS_NETWORK_SOCKET_TASK_H_
