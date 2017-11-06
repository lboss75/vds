/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "private/tcp_network_socket_p.h"
#include "private/read_socket_task_p.h"
#include "private/write_socket_task_p.h"

vds::tcp_network_socket::tcp_network_socket()
: stream_async<uint8_t>(new _tcp_network_socket())
{
}

vds::tcp_network_socket::tcp_network_socket(
    _tcp_network_socket * impl)
: stream_async<uint8_t>(impl)
{
}

vds::tcp_network_socket vds::tcp_network_socket::connect(
  const vds::service_provider& sp,
  const std::string & server,
  const uint16_t port,
  const stream<uint8_t> & input_handler)
{

      auto s = std::make_unique<_tcp_network_socket>(
#ifdef _WIN32
        WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)
#else
        socket(PF_INET, SOCK_STREAM, 0)
#endif//_WIN32
        );

      // Connexion setting for local connexion 
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(server.c_str());
      addr.sin_port = htons(port);

#ifdef _WIN32
      // Connect 
      if (SOCKET_ERROR == ::connect(s->handle(), (struct sockaddr *)&addr, sizeof(addr))) {
        // As we are in non-blocking mode we'll always have the error 
        // WSAEWOULDBLOCK whichis actually not one 
        auto error = WSAGetLastError();
        if (WSAEWOULDBLOCK != error) {
          throw std::system_error(error, std::system_category(), "connect");
        }
      }

      static_cast<_network_service *>(sp.get<inetwork_service>())->associate(s->handle());
#else
      // Connect 
      if (0 > ::connect(s->handle(), (struct sockaddr *)&addr, sizeof(addr))) {
        auto error = errno;
        throw std::system_error(error, std::generic_category(), "connect");
      }
      s->make_socket_non_blocking();
      s->set_timeouts();
#endif
      
      tcp_network_socket sc(s.release());
      sc.start(sp, input_handler);
      return sc;
}


void vds::tcp_network_socket::close()
{
  static_cast<_tcp_network_socket *>(this->impl_.get())->close();
}

void vds::tcp_network_socket::start(
    const vds::service_provider &sp,
    const vds::stream<uint8_t> &input_handler) {
  static_cast<_tcp_network_socket *>(this->impl_.get())->start(sp, input_handler);
}

