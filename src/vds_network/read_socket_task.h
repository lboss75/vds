#ifndef __VDS_NETWORK_READ_SOCKET_TASK_H_
#define __VDS_NETWORK_READ_SOCKET_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "socket_task.h"
#include "network_socket.h"

namespace vds {
  class inetwork_manager;
  
  template<
    typename next_method_type,
    typename error_method_type
  >
  class read_socket_task : public socket_task
  {
  public:
    constexpr static size_t BUFFER_SIZE = 1024;

    read_socket_task(
      next_method_type & next_method,
      error_method_type & error_method,
      const network_socket & s
    ) : s_(s.handle()),
    next_method_(next_method), error_method_(error_method)      
    {
    }
    
    void operator()()
    {
#ifdef _WIN32
      this->wsa_buf_.len = BUFFER_SIZE;
      this->wsa_buf_.buf = (CHAR *)this->buffer_;

      DWORD flags = 0;
      DWORD numberOfBytesRecvd;
      if (NOERROR != WSARecv(this->s_, &this->wsa_buf_, 1, &numberOfBytesRecvd, &flags, &this->overlapped_, NULL)) {
        auto errorCode = WSAGetLastError();
        if (WSA_IO_PENDING != errorCode) {
          throw new std::system_error(errorCode, std::system_category(), "WSARecv failed");
        }
      }
#else//!_WIN32
      event_set(
        &this->event_,
        this->s_,
        EV_READ,
        &read_socket_task::callback,
        this);
      // Schedule client event
      event_add(&this->event_, NULL);
#endif//_WIN32
    }

  private:
    network_socket::SOCKET_HANDLE s_;
    next_method_type & next_method_;
    error_method_type & error_method_;
    u_int8_t buffer_[BUFFER_SIZE];
    
#ifdef _WIN32
    void process(DWORD dwBytesTransfered) override
    {
      this->next_method_(
        *this,
        this->buffer_,
        (size_t)dwBytesTransfered
      );
    }
#else//!_WIN32
    static void callback(int fd, short event, void *arg)
    {
      auto pthis = reinterpret_cast<read_socket_task *>(arg);
      int len = read(fd, pthis->buffer_, BUFFER_SIZE);
      if (len < 0) {
        int error = errno;
        pthis->error_method_(
          new std::system_error(
            error,
            std::system_category()));
        return;
      }
      
      pthis->next_method_(pthis->buffer_, len);
    }
#endif//_WIN32

  };

}

#endif // __VDS_NETWORK_READ_SOCKET_TASK_H_
