#ifndef __VDS_NETWORK_NETWORK_SOCKET_H_
#define __VDS_NETWORK_NETWORK_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "mt_service.h"
#include "network_types.h"

namespace vds {
  class network_socket
  {
  public:
#ifdef _WIN32
    using SOCKET_HANDLE = SOCKET;
#else
    using SOCKET_HANDLE = int;
#endif
      
    network_socket()
#ifdef _WIN32
    : s_(INVALID_SOCKET)
#else
    : s_(-1)
#endif
    {
    }

    network_socket(SOCKET_HANDLE s)
      : s_(s)
    {
#ifdef _WIN32
      if (INVALID_SOCKET == s) {
        auto error = WSAGetLastError();
        throw new std::system_error(error, std::system_category(), "create socket");
      }
#else
      if (s < 0) {
        auto error = errno;
        throw new c_exception("create socket", error);
      }
#endif
    }

    network_socket(const network_socket &) = delete;

    network_socket(network_socket && other)
      : s_(other.detach())
    {
    }
    ~network_socket()
    {
      this->release();
    }

    void release()
    {
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
        closesocket(this->s_);
        this->s_ = INVALID_SOCKET;
      }
#else
      if (0 <= this->s_) {
        shutdown(this->s_, 2);
        this->s_ = -1;
      }
#endif
    }
      
    network_socket & operator = (
      const network_socket &
    ) = delete;

    network_socket & operator = (
      network_socket && other
      )
    {
      this->release();
      this->s_ = other.detach();
      return *this;
    }

    SOCKET_HANDLE handle() const {
#ifdef _WIN32
      if (INVALID_SOCKET == this->s_) {
#else
      if (0 >= this->s_) {
#endif
        throw new std::logic_error("network_socket::handle without open socket");
      }
      return this->s_;
    }

    SOCKET_HANDLE detach() {
      auto result = this->s_;
#ifdef _WIN32
      if (INVALID_SOCKET != this->s_) {
        this->s_ = INVALID_SOCKET;
      }
#else
      if (0 <= this->s_) {
        this->s_ = -1;
      }
#endif
      return result;
    }

  private:
    SOCKET_HANDLE s_;
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_